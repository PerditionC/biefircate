#include <efi.h>
#include <efilib.h>
#include "efi-stuff.h"
#include "loader.h"

extern EFI_HANDLE LibImageHandle;

static UINT64 base_mem_start = 0, base_mem_end = 0;

static UINT64 carve_out_base_mem(unsigned kib)
{
	base_mem_end -= kib * 1024ULL;
	return base_mem_end;
}

void mem_map_init(UINTN *mem_map_key)
{
	EFI_MEMORY_DESCRIPTOR *desc;
	UINTN num_entries = 0, desc_sz;
	UINT32 desc_ver;
	UINT64 addr1 = (UINT64)&desc_ver, addr2, addr3;
	UINT32 ty1 = EfiMaxMemoryType, ty2 = EfiMaxMemoryType,
	       ty3 = EfiMaxMemoryType;
	__asm volatile("movq %%cr3, %0" : "=r" (addr2));
	addr2 &= 0x000ffffffffff000ULL;
	desc = LibMemoryMap(&num_entries, mem_map_key, &desc_sz, &desc_ver);
	if (!num_entries)
		error(u"cannot get memory map!");
	addr3 = (UINT64)desc;
	cputws(u"memory map below 16 MiB:\n"
		"  start    end       type attrs\n");
	while (num_entries-- != 0) {
		EFI_PHYSICAL_ADDRESS start = desc->PhysicalStart,
		    end = start + desc->NumberOfPages * EFI_PAGE_SIZE;
		UINT32 type = desc->Type;
		if (start <= addr1 && addr1 < end)
			ty1 = type;
		if (start <= addr2 && addr2 < end)
			ty2 = type;
		if (start <= addr3 && addr3 < end)
			ty3 = type;
		if (start < 0xffffffUL) {
			cwprintf(u"  0x%06lx 0x%06lx%c %4u 0x%016lx\n", start,
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
				if (start < 0xf0000ULL) {
					if (end > 0xf0000ULL)
						end = 0xf0000ULL;
					if (!base_mem_end) {
						base_mem_start = start;
						base_mem_end = end;
					} else if (end == base_mem_start)
						base_mem_start = start;
					  else if (start == base_mem_end)
						base_mem_end = end;
				}
			}
		}
		/* :-| */
		desc = (EFI_MEMORY_DESCRIPTOR *)((char *)desc + desc_sz);
	}
	if (ty1 != EfiMaxMemoryType)
		cwprintf(u"loader stack mem. (@0x%lx) is type %u\n",
		    addr1, ty1);
	if (ty2 != EfiMaxMemoryType)
		cwprintf(u"page dir. mem. (@0x%lx) is type %u\n",
		    addr2, ty2);
	if (ty3 != EfiMaxMemoryType)
		cwprintf(u"mem. map mem. (@0x%lx) is type %u\n",
		    addr3, ty3);
}

void stage1_done(UINTN mem_map_key)
{
	EFI_STATUS status = BS->ExitBootServices(LibImageHandle, mem_map_key);
	UINT64 reserved_base_mem;
	if (EFI_ERROR(status))
		error_with_status(u"cannot exit UEFI boot services", status);
	fb_con_instate();
	reserved_base_mem = carve_out_base_mem(1);
	lm86_rm86_init((uint16_t)(reserved_base_mem >> 4));
}
