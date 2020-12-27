#include <efi.h>
#include <efilib.h>
#include "efi-stuff.h"
#include "truckload.h"

extern EFI_HANDLE LibImageHandle;

/*
 * Separately maintain
 *   * available physical pages which can be accessed from real mode (up to
 *     1 MiB)
 *   * remaining available physical pages which can be accessed from 32-bit
 *     protected mode (1 MiB--4 GiB)
 *   * remaining available physical pages which can be accessed from 32-bit
 *     protected mode (4 GiB on).
 */
static UINT64 mem20_start = 0, mem20_end = 0,
	      mem32_start = 0, mem32_end = 0,
	      mem64_start = 0, mem64_end = 0;

static UINT64 carve_out_base_mem(unsigned kib)
{
	return mem20_end - 1024ULL;
}

static void record_mem_range(UINT64 start, UINT64 end)
{
	if (start < 0x100000ULL) {
		if (end > 0x100000ULL) {
			record_mem_range(start, 0x100000ULL);
			record_mem_range(end, 0x100000ULL);
		} else if (!mem20_end) {
			mem20_start = start;
			mem20_end = end;
		} else if (start < mem20_start)
			mem20_start = start;
		  else if (end > mem20_end)
			mem20_end = end;
	} else if (start < 0x100000000ULL) {
		if (end > 0x100000000ULL) {
			record_mem_range(start, 0x100000000ULL);
			record_mem_range(end, 0x100000000ULL);
			return;
		} else if (!mem32_end) {
			mem32_start = start;
			mem32_end = end;
		} else if (start < mem32_start)
			mem32_start = start;
		  else if (end > mem32_end)
			mem32_end = end;
	} else {
		if (!mem64_end) {
			mem64_start = start;
			mem64_end = end;
		} else if (start < mem64_start)
			mem64_start = start;
		  else if (end > mem64_end)
			mem64_end = end;
	}
}

void mem_map_init(UINTN *mem_map_key)
{
	EFI_MEMORY_DESCRIPTOR *desc;
	UINTN num_entries = 0, desc_sz;
	UINT32 desc_ver;
	UINT64 addr1, addr2;
	UINT32 ty1 = EfiMaxMemoryType, ty2 = EfiMaxMemoryType;

	/*
	 * Retrieve the memory map.  Designate the memory map's memory as
	 * UEFI boot services data so that we can jettison it after we are
	 * done with boot services.
	 */
	EFI_MEMORY_TYPE save_pool_alloc_type = PoolAllocationType;
	PoolAllocationType = EfiBootServicesData;
	desc = LibMemoryMap(&num_entries, mem_map_key, &desc_sz, &desc_ver);
	PoolAllocationType = save_pool_alloc_type;
	if (!num_entries || !desc_sz)
		error(u"cannot get memory map!");

	/* We got the memory map.  Dump it. */
	addr1 = (UINT64)&desc_ver;
	__asm volatile("movq %%cr3, %0" : "=r" (addr2));
	addr2 &= 0x000ffffffffff000ULL;
	cputws(u"mem. map below 16 MiB:\n"
		"  start     end       type attrs\n");
	while (num_entries-- != 0) {
		EFI_PHYSICAL_ADDRESS start = desc->PhysicalStart,
		    end = start + desc->NumberOfPages * EFI_PAGE_SIZE;
		UINT32 type = desc->Type;
		if (start <= addr1 && addr1 < end)
			ty1 = type;
		if (start <= addr2 && addr2 < end)
			ty2 = type;
		if (start < 0xffffffULL)
			cwprintf(u"  @0x%06lx @0x%06lx%c%4u 0x%016lx\n", start,
			    end > 0x1000000ULL ? (UINT64)0x1000000ULL - 1
					       : end - 1,
			    end > 0x1000000ULL ? u'+' : u' ', type,
			    desc->Attribute);
		switch (type) {
		    default:
			break;
		    case EfiBootServicesCode:
		    case EfiBootServicesData:
		    case EfiConventionalMemory:
		    case EfiACPIReclaimMemory:
		    case EfiACPIMemoryNVS:
			record_mem_range(start, end);
		}
		/* :-| */
		desc = (EFI_MEMORY_DESCRIPTOR *)((char *)desc + desc_sz);
	}

	if (mem20_end)
		cwprintf(u"available base mem. spans @0x%lx--@0x%lx\n",
		    mem20_start, mem20_end - 1);
	if (mem32_end)
		cwprintf(u"available 32-bit ext. mem. spans @0x%lx--@0x%lx\n",
		    mem32_start, mem32_end - 1);
	if (mem64_end)
		cwprintf(u"available 64-bit ext. mem. spans @0x%lx--@0x%lx\n",
		    mem64_start, mem64_end - 1);
	if (ty1 != EfiMaxMemoryType)
		cwprintf(u"loader stack mem. (@0x%lx) is type %u\n",
		    addr1, ty1);
	if (ty2 != EfiMaxMemoryType)
		cwprintf(u"page dir. mem. (@0x%lx) is type %u\n",
		    addr2, ty2);
}

void stage1_done(UINTN mem_map_key)
{
	/*
	 * FIXME: the Red Hat shim (https://github.com/rhboot/shim) says
	 * "System is compromised" & shuts down the system when we try to
	 * call .ExitBootServices(, ) here.
	 *
	 * Meanwhile, PreLoader (https://blog.hansenpartnership.com/linux-
	 * foundation-secure-boot-system-released/) seems to have no problem.
	 * And PreLoader is smaller too...
	 */
	EFI_STATUS status = BS->ExitBootServices(LibImageHandle, mem_map_key);
	UINT64 reserved_base_mem;
	if (EFI_ERROR(status))
		error_with_status(u"cannot exit UEFI boot services", status);
	fb_con_instate();
	reserved_base_mem = carve_out_base_mem(1);
	lm86_rm86_init((uint16_t)(reserved_base_mem >> 4));
}
