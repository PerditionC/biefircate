/*
 * Copyright (c) 2020--2021 TK Chia
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of the developer(s) nor the names of its
 *     contributors may be used to endorse or promote products derived from
 *     this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdbool.h>
#include <string.h>
#include "stage1/stage1.h"
#ifdef XV6_COMPAT
#   include "mp.h"
#endif

extern EFI_HANDLE LibImageHandle;
extern EFI_GUID gEfiLoadedImageProtocolGuid, gEfiGlobalVariableGuid;

static BOOLEAN secure_boot_p = FALSE;
static EFI_HANDLE boot_media_handle;
static uint16_t temp_ebda_seg = 0;

static void init(void)
{
	bmem_init();
	fv_init();
}

static void process_efi_conf_tables(void)
{
	UINTN i, sct_cnt = ST->NumberOfTableEntries;
	Output(u"EFI sys. conf. tables:");
	for (i = 0; i < sct_cnt; ++i) {
		const EFI_CONFIGURATION_TABLE *cft =
		    &ST->ConfigurationTable[i];
		const EFI_GUID *vguid = &cft->VendorGuid;
		if (i % 2 == 0)
			Output(u"\r\n");
		Output(u"  ");
		print_guid(vguid);
	}
	Output(u"\r\n");
}

static void find_boot_media(void)
{
	EFI_STATUS status;
	EFI_LOADED_IMAGE_PROTOCOL *li;
	status = BS->HandleProtocol(LibImageHandle,
	    &gEfiLoadedImageProtocolGuid, (void **)&li);
	if (EFI_ERROR(status))
		error_with_status(u"cannot get EFI_LOADED_IMAGE_PROTOCOL",
		    status);
	boot_media_handle = li->DeviceHandle;
}

static void test_if_secure_boot(void)
{
	UINT8 data = 0;
	UINTN data_sz = sizeof(data);
	EFI_STATUS status = RT->GetVariable(u"SecureBoot",
	    &gEfiGlobalVariableGuid, NULL, &data_sz, &data);
	if (!EFI_ERROR(status) && data)
		secure_boot_p = TRUE;
	Print(u"secure boot: %s\r\n", secure_boot_p ? u"yes" : u"no");
}

static Elf32_Addr alloc_trampoline(void)
{
	EFI_PHYSICAL_ADDRESS addr = 0x100000000ULL;
	EFI_STATUS status = BS->AllocatePages(AllocateMaxAddress,
	    EfiLoaderData, 1, &addr);
	if (EFI_ERROR(status))
		error_with_status(u"cannot get mem. for trampoline & stk.",
		    status);
	Print(u"made space for trampoline & stk. @0x%lx\r\n", addr);
	return (Elf32_Addr)addr;
}

#define STAGE2		u"EFI\\biefirc\\stage2.sys"
#define STAGE2_ALT	u"biefist2.sys"
#define STAGE2_ALT_ALT	u"kernel.sys"

static void dump_stage2_info(EFI_FILE_PROTOCOL *prog, CONST CHAR16 *name)
{
	EFI_FILE_INFO *info = LibFileInfo(prog);
	if (!info)
		error(u"cannot get info on stage 2");
	Print(u"stage2: %s  size: 0x%lx  attrs.: 0x%lx\r\n",
	    name, info->FileSize, info->Attribute);
	FreePool(info);
}

static void read_stage2(EFI_FILE_PROTOCOL *prog, EFI_FILE_PROTOCOL *vol,
			UINTN size, void *buf)
{
	UINTN read_size = size;
	EFI_STATUS status;
	if (!size)
		return;
	status = prog->Read(prog, &read_size, buf);
	if (EFI_ERROR(status)) {
		prog->Close(prog);
		vol->Close(vol);
		error_with_status(u"cannot read stage 2", status);
	}
	if (read_size != size) {
		prog->Close(prog);
		vol->Close(vol);
		error_with_status(u"short read from stage 2", status);
	}
}

static void seek_stage2(EFI_FILE_PROTOCOL *prog, EFI_FILE_PROTOCOL *vol,
			UINT64 pos)
{
	EFI_STATUS status = prog->SetPosition(prog, pos);
	if (EFI_ERROR(status)) {
		prog->Close(prog);
		vol->Close(vol);
		error_with_status(u"cannot seek into stage 2", status);
	}
}

static void free_stage2_mem(const Elf32_Phdr *phdrs, UINT32 ph_cnt)
{
	const Elf32_Phdr *phdr = phdrs;
	while (ph_cnt-- != 0) {
		EFI_PHYSICAL_ADDRESS paddr = phdr->p_paddr;
		UINTN pages =
		  ((UINT64)phdrs->p_memsz + EFI_PAGE_SIZE - 1) / EFI_PAGE_SIZE;
		BS->FreePages(paddr, pages);
		++phdr;
	}
}

static Elf32_Addr load_stage2(void)
{
	enum { MAX_PHDRS = 16 };
	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs;
	CHAR16 *name = STAGE2;
	EFI_FILE_PROTOCOL *vol, *prog;
	EFI_STATUS status;
	Elf32_Ehdr ehdr;
	Elf32_Phdr phdrs[MAX_PHDRS], *phdr;
	UINT32 x1, x2, ph_cnt, ph_idx, entry;
	status = BS->HandleProtocol(boot_media_handle,
	    &gEfiSimpleFileSystemProtocolGuid, (void **)&fs);
	if (EFI_ERROR(status))
		error_with_status(u"cannot get "
		    "EFI_SIMPLE_FILE_SYSTEM_PROTOCOL", status);
	status = fs->OpenVolume(fs, &vol);
	if (EFI_ERROR(status))
		error_with_status(u"cannot get EFI_FILE_PROTOCOL", status);
	status = vol->Open(vol, &prog, name, EFI_FILE_MODE_READ, 0);
	if (EFI_ERROR(status)) {
		name = STAGE2_ALT;
		status = vol->Open(vol, &prog, name, EFI_FILE_MODE_READ, 0);
	}
	if (EFI_ERROR(status)) {
		name = STAGE2_ALT_ALT;
		status = vol->Open(vol, &prog, name, EFI_FILE_MODE_READ, 0);
	}
	if (EFI_ERROR(status)) {
		vol->Close(vol);
		error_with_status(u"cannot open stage 2", status);
	}
	dump_stage2_info(prog, name);
	read_stage2(prog, vol, sizeof ehdr, &ehdr);
	if (ehdr.e_ident[EI_MAG0] != ELFMAG0 ||
	    ehdr.e_ident[EI_MAG1] != ELFMAG1 ||
	    ehdr.e_ident[EI_MAG2] != ELFMAG2 ||
	    ehdr.e_ident[EI_MAG3] != ELFMAG3) {
		Output(u"  not ELF file\r\n");
		goto bad_elf;
	}
	x1 = ehdr.e_ident[EI_VERSION];
	x2 = ehdr.e_version;
	Print(u"  ELF ver.: %u / %u\r\n", x1, x2);
	if (x1 != EV_CURRENT || x2 != EV_CURRENT)
		goto bad_elf;
	x1 = ehdr.e_ehsize;
	x2 = ehdr.e_phentsize;
	ph_cnt = ehdr.e_phnum;
	Print(u"  ehdr sz.: 0x%x  phdr sz.: 0x%x  phdr cnt.: %u\r\n",
	    x1, x2, ph_cnt);
	if (x1 < sizeof(ehdr) || x2 != sizeof(*phdr))
		goto bad_elf;
	if (ph_cnt > MAX_PHDRS) {
		Output(u"  too many phdrs.\r\n");
		goto bad_elf;
	}
	x1 = ehdr.e_machine;
	entry = ehdr.e_entry;
	Print(u"  machine: 0x%x  entry: @0x%x\r\n", x1, entry);
	if (x1 != EM_386) {
		Output(u"  not x86-32 ELF\r\n");
		goto bad_elf;
	}
	seek_stage2(prog, vol, ehdr.e_phoff);
	read_stage2(prog, vol, ph_cnt * sizeof(*phdr), phdrs);
	Output(u"  phdr# file off.  phy.addr.  virt.addr. type       "
		  "file sz.   mem. sz.\r\n");
	for (ph_idx = 0; ph_idx < ph_cnt; ++ph_idx) {
		phdr = &phdrs[ph_idx];
		Elf32_Word type = phdr->p_type;
		Elf32_Off off = phdr->p_offset;
		EFI_PHYSICAL_ADDRESS paddr = phdr->p_paddr;
		Elf32_Word filesz = phdr->p_filesz, memsz = phdr->p_memsz;
		UINTN pages;
		phdr = &phdrs[ph_idx];
		Print(u"  %5u 0x%08x 0x%08lx 0x%08x 0x%08x 0x%08x 0x%08x\r\n",
		    ph_idx, off, paddr, phdr->p_vaddr, type, filesz, memsz);
		if (type != PT_LOAD)
			continue;
		if (filesz > memsz) {
			free_stage2_mem(phdrs, ph_idx);
			Output(u"  seg. file sz. > seg. mem. sz.!\r\n");
			goto bad_elf;
		}
		pages = ((UINT64)memsz + EFI_PAGE_SIZE - 1) / EFI_PAGE_SIZE;
		status = BS->AllocatePages(AllocateAddress,
		    EfiRuntimeServicesData, pages, &paddr);
		if (EFI_ERROR(status)) {
			free_stage2_mem(phdrs, ph_idx);
			error_with_status(u"cannot get mem. for ELF seg.",
			    status);
		}
		seek_stage2(prog, vol, off);
		read_stage2(prog, vol, filesz, (void *)paddr);
		memset((char *)paddr + filesz, 0, memsz - filesz);
	}
	prog->Close(prog);
	vol->Close(vol);
	return entry;
bad_elf:
	prog->Close(prog);
	vol->Close(vol);
	error(u"bad stage2");
	return 0;
}

#ifdef XV6_COMPAT
static uint64_t rdmsr(uint32_t idx)
{
	uint32_t hi, lo;
	__asm volatile("rdmsr" : "=d" (hi), "=a" (lo)
			       : "c" (idx));
	return (uint64_t)hi << 32 | lo;
}

static void cpuid(uint32_t leaf,
		  uint32_t *pa, uint32_t *pb, uint32_t *pc, uint32_t *pd)
{
	uint32_t a, b, c, d;
	__asm volatile("cpuid" : "=a" (a), "=b" (b), "=c" (c), "=d" (d)
			       : "0" (leaf));
	if (pa)
		*pa = a;
	if (pb)
		*pb = b;
	if (pc)
		*pc = c;
	if (pd)
		*pd = d;
}

static void update_cksum(uint8_t *buf, size_t n, uint8_t *p_cksum)
{
	uint8_t *p = buf, cksum = 0;
	*p_cksum = 0;  /* the checksum may be part of the summed area */
	while (n-- != 0)
		cksum -= *p++;
	*p_cksum = cksum;
}

static void fake_mp_table(void)
{
	/*
	 * MIT's Xv6 teaching OS --- to be precise, the x86 version at
	 * https://github.com/mit-pdos/xv6-public --- will panic unless the
	 * Intel multiprocessor configuration tables are set up.
	 *
	 * This code just fashions a minimal set of MP tables to keep Xv6
	 * happy.  The tables are set up in the last KiB of available base
	 * memory.
	 *
	 * The intent is that a more practical ELF32 kernel or bootloader
	 * will obtain the actual multiprocessor setup by other means, such
	 * as ACPI tables.
	 */
	static const char oem_id[8] = "BOCHSCPU", prod_id[12] = "0.1         ";
	typedef struct __attribute__((packed)) {
		unsigned char ebda_kib, ebda_reserved[15];
		intel_mp_flt_t flt;
		intel_mp_conf_hdr_t conf;
		intel_mp_cpu_t cpu;
		intel_mp_ioapic_t ioapic;
	} mp_bundle_t;
	typedef union {
		mp_bundle_t s;
		uint8_t b[sizeof(mp_bundle_t)];
	} mp_bundle_union_t;
	uint32_t cpu_sig, cpu_features;
	/* Get the local APIC address & APIC version. */
	uintptr_t lapic_addr = rdmsr(0x1b) & 0x000ffffffffff000ULL;
	volatile uint32_t *lapic = (volatile uint32_t *)lapic_addr;
	/* Get the pointer to the I/O APIC. */
	volatile uint32_t *ioapic = (volatile uint32_t *)IOAPIC_ADDR;
	/*
	 * Allocate base memory for the MP tables, which will go into a
	 * temporary Extended BIOS Data Area (EBDA).  The MP tables are
	 * expected to be used only at boot time, not run time.
	 */
	mp_bundle_union_t *u = bmem_alloc_boottime(sizeof(mp_bundle_t),
						   PARA_SIZE);
	/* Fill in the length of the temporary EBDA. */
	u->s.ebda_kib = 1;
	memset(u->s.ebda_reserved, 0, 15);
	/* Fill up the MP floating pointer structure. */
	Print(u"placing MP tables @0x%x\r\n", (uintptr_t)u);
	u->s.flt.sig = MAGIC32('_', 'M', 'P', '_');
	u->s.flt.mp_conf_addr = (uint32_t)(uintptr_t)&u->s.conf;
	u->s.flt.len = sizeof(u->s.flt);
	u->s.flt.spec_rev = 4;
	u->s.flt.features[0] = u->s.flt.features[2] =
	    u->s.flt.features[3] = u->s.flt.features[4] = 0;
	u->s.flt.features[1] = MP_FEAT1_IMCRP;
	update_cksum(u->b + offsetof(mp_bundle_t, flt), sizeof(u->s.flt),
	    &u->s.flt.cksum);
	/*
	 * Fill up the MP configuration table header (except for the
	 * checksum).
	 */
	u->s.conf.sig = MAGIC32('P', 'C', 'M', 'P');
	u->s.conf.base_tbl_len = sizeof(u->s.conf) + sizeof(u->s.cpu) +
				 sizeof(u->s.ioapic);
	u->s.conf.spec_rev = 4;
	memcpy(u->s.conf.oem_id, oem_id, sizeof oem_id);
	memcpy(u->s.conf.prod_id, prod_id, sizeof prod_id);
	u->s.conf.oem_tbl_ptr = 0;
	u->s.conf.oem_tbl_sz = 0;
	u->s.conf.ent_cnt = 2;
	u->s.conf.lapic_addr = (uint32_t)lapic_addr;
	u->s.conf.ext_tbl_len = 0;
	u->s.conf.ext_tbl_cksum = u->s.conf.reserved = 0;
	/* While at it... */
	Print(u"LAPIC: @0x%lx\r\n", lapic_addr);
	/* Fill up the entry for this processor. */
	u->s.cpu.type = MP_CPU;
	u->s.cpu.lapic_id = (uint8_t)lapic[0x20 / 4];
	u->s.cpu.lapic_ver = (uint8_t)lapic[0x30 / 4];
	u->s.cpu.cpu_flags = MP_CPU_EN | MP_CPU_BP;
	cpuid(1, &cpu_sig, NULL, NULL, &cpu_features);
	u->s.cpu.cpu_sig = cpu_sig;
	u->s.cpu.cpu_features = cpu_features;
	/* Fill up the entry for the (first) I/O APIC. */
	u->s.ioapic.type = MP_IOAPIC;
	ioapic[0] = 0x00;
	u->s.ioapic.ic_id = (uint8_t)(ioapic[0x10 / 4] >> 24);
	ioapic[0] = 0x01;
	u->s.ioapic.ic_ver = (uint8_t)ioapic[0x10 / 4];
	u->s.ioapic.ic_flags = MP_IOAPIC_EN;
	u->s.ioapic.ic_addr = (uint32_t)IOAPIC_ADDR;
	/* Compute the checksum for the MP configuration table. */
	update_cksum(u->b + offsetof(mp_bundle_t, conf),
		     sizeof(u->s.conf) + sizeof(u->s.cpu) +
		     sizeof(u->s.ioapic), &u->s.conf.cksum);
	/* Set the temporary EBDA segment. */
	temp_ebda_seg = (uint16_t)((uintptr_t)u / PARA_SIZE);
}
#endif

static void get_time(EFI_TIME *when)
{
	EFI_STATUS status = RT->GetTime(when, NULL);
	if (EFI_ERROR(status))
		error_with_status(u"cannot get time", status);
}

static void wait_for_time_change(void)
{
	EFI_TIME then, now;
	get_time(&then);
	do {
		hlt();
		get_time(&now);
	} while (now.Second == then.Second &&
		 now.Nanosecond == then.Nanosecond);
}

static void wait_for_one_second(void)
{
	EFI_TIME then, now;
	UINT64 then_ns, now_ns;
	get_time(&then);
	then_ns = 1000000000ULL * then.Second + then.Nanosecond;
	do {
		hlt();
		get_time(&now);
		now_ns = 1000000000ULL * now.Second + now.Nanosecond;
		if (now.Minute != then.Minute)
			now_ns += 60000000000ULL;
	} while (now_ns < then_ns + 1000000000ULL);
}

static unsigned prepare_to_hand_over(EFI_HANDLE image_handle)
{
	enum { EfiPersistentMemory = EfiPalCode + 1 };
	bdat_bmem_t *bd;
	uint32_t boottime_bmem_bot, runtime_bmem_top;
	EFI_MEMORY_DESCRIPTOR *descs, *desc;
	UINTN num_ents = 0, map_key, desc_sz, ent_iter;
	EFI_STATUS status;
	/* Wrap up firmware volume handling. */
	fv_fini();
	/* Say we are about to exit UEFI. */
	Output(u"exit UEFI\r\n");
	/*
	 * Add information about blocks of extended memory (above the 1 MiB
	 * mark) to the boot parameters.
	 */
	descs = get_mem_map(&num_ents, &map_key, &desc_sz);
	FOR_EACH_MEM_DESC(desc, descs, desc_sz, num_ents, ent_iter) {
		EFI_PHYSICAL_ADDRESS start = desc->PhysicalStart, end;
		uint32_t e820_type;
		end = start + desc->NumberOfPages * EFI_PAGE_SIZE;
		if (end <= BMEM_MAX_ADDR && end != 0)
			continue;
		if (start < BMEM_MAX_ADDR)
			start = BMEM_MAX_ADDR;
		switch (desc->Type) {
		    case EfiLoaderCode:
		    case EfiLoaderData:
		    case EfiBootServicesCode:
		    case EfiBootServicesData:
		    case EfiConventionalMemory:
			e820_type = E820_RAM;	break;
		    case EfiACPIReclaimMemory:
			e820_type = E820_ACPI;  break;
		    case EfiACPIMemoryNVS:
			e820_type = E820_NVS;   break;
		    case EfiPersistentMemory:
			e820_type = E820_PMEM;  break;
		    default:
			e820_type = E820_RESERVED;
		}
		bparm_add_mem_range(start, end - start, e820_type, 1U);
	}
	/*
	 * Wrap up base memory handling.  Add boot parameters to tell the
	 * bootloader about base memory availability at boot time & run
	 * time.
	 *
	 * We can only do this after adding the boot parameters for the map
	 * of extended memory, because these latter parameters will
	 * themselves be allocated in base memory (via bmem.c).
	 */
	bd = bparm_add(BP_BMEM, sizeof(bdat_bmem_t));
	bmem_fini(descs, num_ents, desc_sz,
	    &boottime_bmem_bot, &runtime_bmem_top);
	bd->boottime_bmem_bot_seg = addr_to_rm_seg(boottime_bmem_bot);
	bd->runtime_bmem_top_seg = addr_to_rm_seg(runtime_bmem_top);
	/* Wait for about 3 seconds. */
	wait_for_time_change();
	wait_for_one_second();
	wait_for_one_second();
	wait_for_one_second();
	/* Really exit boot services... */
	status = BS->ExitBootServices(image_handle, map_key);
	if (EFI_ERROR(status))
		error_with_status(u"cannot exit UEFI", status);
	return runtime_bmem_top / KIBYTE;
}

EFI_STATUS efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *system_table)
{
	Elf32_Addr trampoline, entry;
	unsigned base_kib;
	InitializeLib(image_handle, system_table);
	Output(u".:. biefircate " VERSION " .:.\r\n");
	init();
	process_efi_conf_tables();
	find_boot_media();
	test_if_secure_boot();
	process_pci();
	trampoline = alloc_trampoline();
	entry = load_stage2();
#ifdef XV6_COMPAT
	fake_mp_table();
#endif
	base_kib = prepare_to_hand_over(image_handle);
	run_stage2(entry, trampoline, base_kib, temp_ebda_seg, bparm_get());
	return 0;
}
