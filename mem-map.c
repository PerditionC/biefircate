/*
 * Copyright (c) 2020--2021 TK Chia
 *
 * This file is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */

#include <efi.h>
#include <efilib.h>
#include <inttypes.h>
#include <string.h>
#include "efi-stuff.h"
#include "truckload.h"

extern EFI_HANDLE LibImageHandle;

#define E820_AVAILABLE		0x00000001U
#define E820_RESERVED		0x00000002U
#define E820_ACPI_RECLAIM	0x00000003U
#define E820_ACPI_NVS		0x00000004U
#define E820_UNUSABLE		0x00000005U	/* per Intel EDK II */

/*
 * Memory map address range descriptors much in the style of BIOS int 0x15,
 * ax = 0xe820 (see Ralf Brown's Interrupt List).  For now, these
 * descriptors are maintained in a linked list sorted in _decreasing_ order
 * of memory address.
 */

typedef struct mem_range mem_range_t;

struct mem_range {
	struct mem_range *next;
	uint64_t start, size;
	uint32_t e820_type;
};

static mem_range_t *mem_ranges = NULL;

static mem_range_t *do_record_mem_range(mem_range_t *prev, mem_range_t *next,
    uint64_t start, uint64_t size, uint32_t e820_type)
{
	if (prev && prev->e820_type == e820_type &&
		    prev->start == start + size) {
		/* Coalesce with *prev. */
		prev->start = start;
		prev->size += size;
		return prev;
	} else if (next && next->e820_type == e820_type &&
		    next->start + next->size == start) {
		/* Coalesce with *next. */
		next->size += size;
		return next;
	} else {
		/*
		 * Cannot coalesce with an existing descriptor.  Insert a new
		 * descriptor between (maybe) *prev & (maybe) *next.
		 */
		mem_range_t *curr = mem_heap_alloc(sizeof(mem_range_t));
		*curr = (mem_range_t){ next, start, size, e820_type };
		if (prev)
			prev->next = curr;
		else
			mem_ranges = curr;
		return curr;
	}
}

static void do_delete_mem_range(mem_range_t *prev, mem_range_t *curr)
{
	if (prev)
		prev->next = curr->next;
	else
		mem_ranges = curr->next;
	mem_heap_free(curr);
}

static void record_mem_range(uint64_t start, uint64_t end, UINT32 efi_type)
{
	uint32_t e820_type;
	uint64_t size = end - start;
	mem_range_t *prev, *next;
	if (!size)
		return;
	switch (efi_type) {
	    case EfiBootServicesCode:
	    case EfiBootServicesData:
	    case EfiRuntimeServicesCode:
	    case EfiRuntimeServicesData:
	    case EfiConventionalMemory:
		e820_type = E820_AVAILABLE;
		break;
	    case EfiUnusableMemory:
		e820_type = E820_UNUSABLE;
		break;
	    case EfiACPIReclaimMemory:
		e820_type = E820_ACPI_RECLAIM;
		break;
	    case EfiACPIMemoryNVS:
		e820_type = E820_ACPI_NVS;
		break;
	    default:
		e820_type = E820_RESERVED;
	}
	prev = NULL;
	next = mem_ranges;
	while (next && next->start > start) {
		prev = next;
		next = next->next;
	}
	do_record_mem_range(prev, next, start, size, e820_type);
}

void mem_map_init(UINTN *mem_map_key)
{
	EFI_MEMORY_DESCRIPTOR *descs, *desc;
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
	descs = LibMemoryMap(&num_entries, mem_map_key, &desc_sz, &desc_ver);
	PoolAllocationType = save_pool_alloc_type;
	if (!num_entries || !desc_sz)
		panic("cannot get memory map!");

	/*
	 * We got the memory map.  Dump it, & also compact it & convert it
	 * into our internal format.
	 */
	addr1 = (UINT64)&desc_ver;
	__asm volatile("movq %%cr3, %0" : "=r" (addr2));
	addr2 &= 0x000ffffffffff000ULL;
	cputs("mem. map below 16 MiB:\n"
	      "  start     end       type attrs\n");
	desc = descs;
	while (num_entries-- != 0) {
		EFI_PHYSICAL_ADDRESS start = desc->PhysicalStart,
		    end = start + desc->NumberOfPages * EFI_PAGE_SIZE;
		UINT32 type = desc->Type;
		if (start <= addr1 && addr1 < end)
			ty1 = type;
		if (start <= addr2 && addr2 < end)
			ty2 = type;
		if (start < 0xffffffULL)
			cprintf("  @0x%06" PRIx64 " @0x%06" PRIx64 "%c"
				"%4u 0x%016" PRIx64 "\n",
			    start,
			    end > 0x1000000ULL ? (UINT64)0x1000000ULL - 1
					       : end - 1,
			    end > 0x1000000ULL ? '+' : ' ',
			    type,
			    desc->Attribute);
		record_mem_range(start, end, type);
		/* :-| */
		desc = (EFI_MEMORY_DESCRIPTOR *)((char *)desc + desc_sz);
	}

	if (ty1 != EfiMaxMemoryType)
		cprintf("loader stack mem. (@%p) is type %u\n",
		    (void *)addr1, ty1);
	if (ty2 != EfiMaxMemoryType)
		cprintf("page dir. mem. (@%p) is type %u\n",
		    (void *)addr2, ty2);
}

void *mem_map_reserve_page(uint64_t max_end_addr)
{
	uint64_t pg;
	mem_range_t *prev = NULL, *curr = mem_ranges;
	while (curr && (curr->start >= max_end_addr ||
			curr->e820_type != E820_AVAILABLE))
	{
		prev = curr;
		curr = curr->next;
	}
	if (!curr)
		panic("failed to reserve a mem. pg. below @%#" PRIx64,
		    max_end_addr);
	/*
	 * Carve out the page at the highest available address below
	 * max_addr.  If there is a page at the beginning of the memory
	 * range which is exactly at {max_end_addr - EFI_PAGE_SIZE}, or at
	 * the end of the memory range & before max_end_addr, then things
	 * are slightly easier.  Otherwise, we need to split the range into
	 * 3 ranges.
	 */
	if (curr->start + curr->size <= max_end_addr) {
		pg = curr->start + curr->size - EFI_PAGE_SIZE;
		if (curr->size == EFI_PAGE_SIZE)
			do_delete_mem_range(prev, curr);
		else
			curr->size -= EFI_PAGE_SIZE;
		do_record_mem_range(prev, curr, pg, EFI_PAGE_SIZE,
		    E820_RESERVED);
	} else if (curr->start == max_end_addr - EFI_PAGE_SIZE) {
		pg = curr->start;
		curr->size -= EFI_PAGE_SIZE;
		curr->start += EFI_PAGE_SIZE;
		do_record_mem_range(curr, curr->next, pg, EFI_PAGE_SIZE,
		    E820_RESERVED);
		return (void *)pg;
	} else {
		uint64_t old_end = curr->start + curr->size;
		pg = max_end_addr - EFI_PAGE_SIZE;
		curr->size = pg - curr->start;
		curr = do_record_mem_range(prev, curr,
		    max_end_addr, old_end - max_end_addr, E820_AVAILABLE);
		do_record_mem_range(prev, curr, pg, EFI_PAGE_SIZE,
		    E820_RESERVED);
	}
	memset((void *)pg, 0, EFI_PAGE_SIZE);
	return (void *)pg;
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
	if (EFI_ERROR(status))
		panic_efi("cannot exit UEFI boot services", status);
	BS = NULL;
}
