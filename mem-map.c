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
#define E820_BS_DATA		0xb5b5d474U	/* temporary marker for boot
						   services data; will be
						   turned into E820_AVAILABLE
						   once we set up our own
						   page tables */

/*
 * Memory map address range descriptors much in the style of BIOS int 0x15,
 * ax = 0xe820 (see Ralf Brown's Interrupt List).  For now, these
 * descriptors are maintained in a linked list sorted in _descending_ order
 * of memory address.
 */

typedef struct mem_type mem_type_t;

struct mem_type {
	struct mem_type *next;
	uint64_t start, size;
	uint32_t e820_type;
};

/*
 * Record of the cacheability of memory ranges.  Only memory ranges which
 * are either write-through or simply uncacheable are listed here.
 *
 * This record is used during later initialization to build up the new
 * runtime page tables for long mode.  This is also a linked list in
 * descending order.
 */

typedef struct mem_cacheability mem_cacheability_t;

struct mem_cacheability {
	struct mem_cacheability *next;
	uint64_t start, size;
	bool writethru_p;
};

static mem_type_t *mem_types = NULL;
static INIT_DATA mem_cacheability_t *mem_cacheabilities = NULL;

static INIT_TEXT mem_type_t *do_record_mem_type(mem_type_t *prevprev,
    mem_type_t *prev, mem_type_t *next, uint64_t start, uint64_t size,
    uint32_t e820_type)
{
	if (prev && prev->e820_type == e820_type &&
		    prev->start == start + size) {
		if (next && next->e820_type == e820_type &&
		    next->start + next->size == start) {
			/* Coalesce with both *prev & *next. */
			next->size += size + prev->size;
			if (prevprev)
				prevprev->next = next;
			else
				mem_types = next;
			mem_heap_free(prev);
			return next;
		} else {
			/* Coalesce with *prev. */
			prev->start = start;
			prev->size += size;
			return prev;
		}
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
		mem_type_t *curr = mem_heap_alloc(sizeof(mem_type_t));
		*curr = (mem_type_t){ next, start, size, e820_type };
		if (prev)
			prev->next = curr;
		else
			mem_types = curr;
		return curr;
	}
}

static INIT_TEXT mem_type_t *do_delete_mem_type(mem_type_t *prev,
    mem_type_t *curr)
{
	mem_type_t *next = curr->next;
	if (prev)
		prev->next = next;
	else
		mem_types = next;
	mem_heap_free(curr);
	return next;
}

static INIT_TEXT void record_mem_type(uint64_t start, uint64_t end,
    UINT32 efi_type)
{
	uint32_t e820_type;
	uint64_t size = end - start;
	mem_type_t *prevprev, *prev, *next;
	if (!size)
		return;
	switch (efi_type) {
	    case EfiBootServicesCode:
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
	    case EfiBootServicesData:
		e820_type = E820_BS_DATA;
		break;
	    default:
		e820_type = E820_RESERVED;
	}
	prevprev = prev = NULL;
	next = mem_types;
	while (next && next->start > start) {
		prevprev = prev;
		prev = next;
		next = next->next;
	}
	do_record_mem_type(prevprev, prev, next, start, size, e820_type);
}

static INIT_TEXT void do_record_mem_cacheability(mem_cacheability_t *prevprev,
    mem_cacheability_t *prev, mem_cacheability_t *next,
    uint64_t start, uint64_t size, bool writethru_p)
{
	if (prev && prev->writethru_p == writethru_p &&
		    prev->start == start + size) {
		if (next && next->writethru_p == writethru_p &&
			    next->start + next->size == start) {
			/* Coalesce with both *prev & *next. */
			next->size += size + prev->size;
			if (prevprev)
				prevprev->next = next;
			else
				mem_cacheabilities = next;
			mem_heap_free(prev);
		} else {
			/* Coalesce with *prev. */
			prev->start = start;
			prev->size += size;
		}
	} else if (next && next->writethru_p == writethru_p &&
			   next->start + next->size == start) {
		/* Coalesce with *next. */
		next->size += size;
	} else {
		mem_cacheability_t *curr =
		    mem_heap_alloc(sizeof(mem_cacheability_t));
		*curr = (mem_cacheability_t){ next, start, size, writethru_p };
		if (prev)
			prev->next = curr;
		else
			mem_cacheabilities = curr;
	}
}

static INIT_TEXT void record_mem_cacheability(uint64_t start, uint64_t end,
    bool writeback_p, bool writethru_p)
{
	uint64_t size = end - start;
	mem_cacheability_t *prevprev, *prev, *next;
	if (!size)
		return;
	if (writeback_p && writethru_p)
		return;
	prevprev = prev = NULL;
	next = mem_cacheabilities;
	while (next && next->start > start) {
		prevprev = prev;
		prev = next;
		next = next->next;
	}
	do_record_mem_cacheability(prevprev, prev, next,
	    start, size, writethru_p);
}

INIT_TEXT void mem_map_init(UINTN *mem_map_key)
{
	EFI_MEMORY_DESCRIPTOR *descs, *desc;
	UINTN num_entries = 0, desc_sz;
	UINT32 desc_ver;
	UINT64 addr1, addr2;
	UINT32 ty1 = EfiMaxMemoryType, ty2 = EfiMaxMemoryType;
	mem_cacheability_t *mc;

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
	 * into our internal formats.
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
		UINT64 attribute = desc->Attribute;
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
		if (start >= 0x00007ffffffff000ULL) {
			cprintf("warning: ignoring mem. @%p--@%p above "
				"47-bit addr. space", start, end);
		} else {
			if (end > 0x00007ffffffff000ULL) {
				cprintf("warning: ignoring mem. @%p--@%p "
					"above 47-bit addr. space",
				    (void *)0x000ffffffffff000ULL, end);
				end = 0x00007ffffffff000ULL;
			}
			record_mem_type(start, end, type);
			record_mem_cacheability(start, end,
			    (attribute & EFI_MEMORY_WB) != 0,
			    (attribute & EFI_MEMORY_WT) != 0);
		}
		/* :-| */
		desc = (EFI_MEMORY_DESCRIPTOR *)((char *)desc + desc_sz);
	}

	/*
	 * Also dump information on memory ranges which are only write-through
	 * or uncacheable.
	 */
	mc = mem_cacheabilities;
	if (!mc)
		cputs("all mem. is cacheable as write-back!\n");
	else {
		cputs("write-thru. or uncacheable mem.:\n"
		      "  start               end                 w-t.?\n");
		while (mc) {
			cprintf("  @0x%016x @0x%016x %c\n",
			    mc->start, mc->start + mc->size - 1,
			    mc->writethru_p ? 'y' : 'n');
			mc = mc->next;
		}
	}

	if (ty1 != EfiMaxMemoryType)
		cprintf("loader stack mem. (@%p) is type %u\n",
		    (void *)addr1, ty1);
	if (ty2 != EfiMaxMemoryType)
		cprintf("page dir. mem. (@%p) is type %u\n",
		    (void *)addr2, ty2);
}

INIT_TEXT void *mem_map_reserve_page(uint64_t max_end_addr)
{
	uint64_t pg;
	mem_type_t *prevprev = NULL, *prev = NULL, *curr = mem_types;
	max_end_addr &= -PAGE_SIZE;
	while (curr && (curr->start >= max_end_addr ||
			curr->e820_type != E820_AVAILABLE))
	{
		prevprev = prev;
		prev = curr;
		curr = curr->next;
	}
	if (!curr)
		panic("failed to reserve a mem. pg. below @%#" PRIx64,
		    max_end_addr);
	/*
	 * Carve out the page at the highest available address below
	 * max_addr.  If there is a page at the beginning of the memory
	 * range which is exactly at {max_end_addr - PAGE_SIZE}, or at the
	 * end of the memory range & before max_end_addr, then things are
	 * slightly easier.  Otherwise, we need to split the range into 3
	 * ranges.
	 */
	if (curr->start + curr->size <= max_end_addr) {
		pg = curr->start + curr->size - PAGE_SIZE;
		if (curr->size == EFI_PAGE_SIZE)
			curr = do_delete_mem_type(prev, curr);
		else
			curr->size -= EFI_PAGE_SIZE;
		do_record_mem_type(prevprev, prev, curr, pg, PAGE_SIZE,
		    E820_RESERVED);
	} else if (curr->start == max_end_addr - PAGE_SIZE) {
		pg = curr->start;
		curr->size -= PAGE_SIZE;
		curr->start += PAGE_SIZE;
		do_record_mem_type(prev, curr, curr->next, pg, PAGE_SIZE,
		    E820_RESERVED);
		return (void *)pg;
	} else {
		uint64_t old_end = curr->start + curr->size;
		pg = max_end_addr - PAGE_SIZE;
		curr->size = pg - curr->start;
		curr = do_record_mem_type(prevprev, prev, curr,
		    max_end_addr, old_end - max_end_addr, E820_AVAILABLE);
		do_record_mem_type(prevprev, prev, curr, pg, PAGE_SIZE,
		    E820_RESERVED);
	}
	memset((void *)pg, 0, PAGE_SIZE);
	return (void *)pg;
}

INIT_TEXT void *mem_map_reserve_page_anywhere(void)
{
	return mem_map_reserve_page(~0ULL);
}

/*
 * Return the address beyond the last byte of memory covered by the memory
 * map.
 */
INIT_TEXT uint64_t mem_map_all_end(void)
{
	mem_type_t *mt = mem_types;
	/* OVMF does not include the video frame buffer in the memory map. */
	uint64_t end1 = fb_con_mem_end(), end2 = 0;
	end1 = (end1 + PAGE_SIZE - 1) & -PAGE_SIZE;
	if (mt)
		end2 = mt->start + mt->size;
	return end1 > end2 ? end1 : end2;
}

/* Free all UEFI boot services pages & mark them E820_AVAILABLE. */
INIT_TEXT void mem_map_free_bs(void)
{
	mem_type_t *prevprev = NULL, *prev = NULL, *curr = mem_types;
	cputs("freeing UEFI boot services memory\n");
	while (curr) {
		if (curr->e820_type == E820_BS_DATA) {
			uint64_t start = curr->start, size = curr->size;
			curr = do_delete_mem_type(prev, curr);
			do_record_mem_type(prevprev, prev, curr,
			    start, size, E820_AVAILABLE);
		}
		prevprev = prev;
		prev = curr;
		curr = curr->next;
	}
}

/*
 * Return cacheability properties of the memory at the given address.  Also
 * return the address beyond the end of the containing memory block which
 * has the same cacheability.
 */
INIT_TEXT void mem_map_get_cacheability(uint64_t addr, uint64_t *end,
    bool *writeback_p, bool *writethru_p)
{
	const mem_cacheability_t *prev = NULL, *curr = mem_cacheabilities;
	while (curr && curr->start > addr) {
		prev = curr;
		curr = curr->next;
	}
	if (curr && addr < curr->start + curr->size) {
		/* addr lies inside *curr. */
		if (writeback_p)
			*writeback_p = false;
		if (writethru_p)
			*writethru_p = curr->writethru_p;
		if (end)
			*end = curr->start + curr->size;
	} else {
		/* addr lies between start of *prev and end of *curr. */
		if (writeback_p)
			*writeback_p = true;
		if (writethru_p)
			*writethru_p = true;
		if (end) {
			if (prev)
				*end = prev->start;
			else
				*end = ~0ULL;
		}
	}
}

INIT_TEXT void stage1_done(UINTN mem_map_key)
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
	EFI_STATUS status;
	cputs("exiting UEFI boot services\n");
	status = BS->ExitBootServices(LibImageHandle, mem_map_key);
	if (EFI_ERROR(status))
		panic_efi("cannot exit UEFI boot services", status);
	BS = NULL;
}
