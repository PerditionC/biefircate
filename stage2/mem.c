/*
 * Copyright (c) 2021 TK Chia
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

#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "stage2/stage2.h"

#define EFI_MEMORY_WT	(1ULL <<  2)
#define EFI_MEMORY_WB	(1ULL <<  3)

/* Structure for a range of (unused) 32-bit virtual addresses. */
typedef struct {
	uint32_t start, len;
} va_range_t;

static unsigned num_mem_ranges = 0, max_mem_ranges = 0,
		num_unused_va_ranges = 0, max_unused_va_ranges = 0;
static mem_range_t *mem_ranges;
static va_range_t *unused_va_ranges;
static uint64_t *pdpt = NULL;

static void shellsort(mem_range_t *mrs, unsigned nmr)
{
	/* See Sedgewick 1996 (https://www.cs.princeton.edu/~rs/shell/). */
	unsigned h = 1, i, j;
	if (nmr < 2)
		return;
	while (h < nmr)
		h *= 2;
	--h;
	do {
		for (i = h; i < nmr; ++i) {
			mem_range_t v = mrs[i];
			j = i;
			while (j > h && mrs[j - h].start > v.start) {
				mrs[j] = mrs[j - h];
				j -= h;
			}
			mrs[j] = v;
		}
		h >>= 1;
	} while (h);
}

static mem_range_t *find_highest_range_below(uint64_t max_addr)
{
	unsigned left = 0, right = num_mem_ranges;
	mem_range_t *mr;
	while (left < right - 1) {
		unsigned mid = (left + right) / 2;
		mr = &mem_ranges[mid];
		if (mr->start + mr->len - 1 > max_addr - 1)
			right = mid;
		else
			left = mid;
	}
	mr = &mem_ranges[left];
	while (mr->start + mr->len - 1 > max_addr - 1) {
		if (!left)
			hlt();
		--mr;
		--left;
	}
	return mr;
}

static void split_range(mem_range_t *mr, uint64_t split_point,
    uint32_t e820_type_below, uint32_t e820_type_above)
{
	if (mr->start == split_point)
		mr->e820_type = e820_type_above;
	else if (mr->start + mr->len == split_point)
		mr->e820_type = e820_type_below;
	else {
		mem_range_t *mr2;
		if (num_mem_ranges >= max_mem_ranges)
			hlt();
		mr2 = &mem_ranges[num_mem_ranges];
		++num_mem_ranges;
		/* Ugh.  Hopefully we will not need to do this too often. */
		while (mr2 != mr) {
			*mr2 = mr2[-1];
			--mr2;
		}
		++mr2;
		mr->len = split_point - mr->start;
		mr2->len -= mr->len;
		mr2->start = split_point;
		mr->e820_type = e820_type_below;
		mr2->e820_type = e820_type_above;
	}
}

static void mem_map_init(bparm_t *bparms)
{
	unsigned nmr, mmr;
	size_t e820_need_space;
	bparm_t *bp;
	bdat_mem_range_t *bdmr, *bdmr_chosen = NULL;
	mem_range_t *mrs, *mr;
	/*
	 * Copy the memory map passed in the stage 1 boot parameters to
	 * extended memory.  First find the number of memory address ranges
	 * given in the boot parameters, & the amount of space needed to
	 * record these ranges.
	 *
	 * Also allocate some extra space for the memory map beyond the
	 * current number of entries, to allow for some memory blocks to be
	 * split into two later.
	 */
	mmr = 1;
	for (bp = bparms; bp; bp = bp->next)
		if (bp->type == BP_MRNG && bp->u->mem_range.len != 0)
			++mmr;
	mmr = (3 * mmr + 1) / 2;
	if (mmr < 16)
		mmr = 16;
	e820_need_space = mmr * sizeof(mem_range_t);
	/*
	 * Find an area of memory that can store the entire memory map & is
	 * below the 4 GiB mark.  Try to store the memory map as high in
	 * extended memory as possible.
	 */
	for (bp = bparms; bp; bp = bp->next) {
		uint64_t start, len;
		if (bp->type != BP_MRNG)
			continue;
		bdmr = &bp->u->mem_range;
		start = bdmr->start;
		len = bdmr->len;
		if (bdmr->e820_type != E820_RAM ||
		    start >= XM32_MAX_ADDR ||
		    len > XM32_MAX_ADDR - start ||
		    len < e820_need_space)
			continue;
		if (!bdmr_chosen || start > bdmr_chosen->start)
			bdmr_chosen = bdmr;
	}
	if (!bdmr_chosen)
		hlt();
	/* Carve out memory for the memory map. */
	mrs = (mem_range_t *)(uintptr_t)
		  (bdmr_chosen->start + bdmr_chosen->len - e820_need_space);
	/* Copy out the memory map.  Discard memory ranges of length zero. */
	mr = mrs;
	nmr = 0;
	for (bp = bparms; bp; bp = bp->next) {
		if (bp->type != BP_MRNG)
			continue;
		bdmr = &bp->u->mem_range;
		if (!bdmr->len)
			continue;
		mr->start = bdmr->start;
		mr->len = bdmr->len;
		mr->e820_type = bdmr->e820_type;
		mr->e820_ext_attr = bdmr->e820_ext_attr;
		mr->uefi_attr = bdmr->uefi_attr;
		if (bdmr == bdmr_chosen) {
			mr->len -= e820_need_space;
			if (mr->len) {
				++mr;
				++nmr;
			}
			mr->start = (uint64_t)(uintptr_t)mrs;
			mr->len = e820_need_space;
			mr->e820_type = E820_RESERVED;
			mr->e820_ext_attr = bdmr->e820_ext_attr;
			mr->uefi_attr = bdmr->uefi_attr;
		}
		++mr;
		++nmr;
	}
	/* Sort the memory map by order of increasing ending addresses. */
	shellsort(mrs, nmr);
	/* Remember the memory map. */
	mem_ranges = mrs;
	num_mem_ranges = nmr;
	max_mem_ranges = mmr;
}

/*
 * Make sure that the PDE *`pde' refers to a bottom-level PT.  If *`pde' is
 * a large page, turn it into a PT with several small pages.  If *`pde' is a
 * blank entry, allocate a new PT.
 *
 * Return a pointer to the PT.
 */
static uint64_t *ensure_pt(uint64_t *p_pde)
{
	uint64_t pde = *p_pde, *pt, pte;
	unsigned i;
	if (pde) {
		if ((pde & PDE_PS) == 0) {
			pt = (uint64_t *)((uint32_t)pde & -PAGE_SIZE);
			return pt;
		}
		pt = mem_alloc(PAGE_SIZE, PAGE_SIZE, 0);
		pte = pde & ~(uint64_t)PDE_PS;
		for (i = 0; i <= 0x1ff; ++i) {
			pt[i] = pte;
			pte += PAGE_SIZE;
		}
	} else {
		pt = mem_alloc(PAGE_SIZE, PAGE_SIZE, 0);
		memset(pt, 0, PAGE_SIZE);
	}
	pde = (uint32_t)pt | PTE_P | PTE_RW | PTE_US;
	*p_pde = pde;
	return pt;
}

static void do_va_map(uint32_t vstart, uint64_t pstart, uint32_t len,
    unsigned pte_flags)
{
	while (len) {
		unsigned pdpti = vstart >> 30,
			 pdi = (vstart >> 21) & 0x1ff;
		uint32_t pdpte = (uint32_t)pdpt[pdpti];
		uint64_t *pd, pde, *pt;
		pd = (uint64_t *)((uint32_t)pdpte & -PDPT_ALIGN);
		if (vstart % LARGE_PAGE_SIZE == 0 &&
		    pstart % LARGE_PAGE_SIZE == 0 &&
		    len >= LARGE_PAGE_SIZE &&
		    (pd[pdi] == 0 || (pd[pdi] & PDE_PS) != 0)) {
			pde = pstart | PTE_P | PTE_RW | PTE_US | PDE_PS |
			    pte_flags;
			pd[pdi] = pde;
			vstart += LARGE_PAGE_SIZE;
			pstart += LARGE_PAGE_SIZE;
			len -= LARGE_PAGE_SIZE;
		} else {
			unsigned pti = (vstart >> 12) & 0x1ff;
			pt = ensure_pt(&pd[pdi]);
			pt[pti] = pstart | PTE_P | PTE_RW | PTE_US | pte_flags;
			vstart += PAGE_SIZE;
			pstart += PAGE_SIZE;
			len -= PAGE_SIZE;
		}
	}
}

static void do_va_id_map(uint32_t start, uint32_t len, unsigned pte_flags)
{
	do_va_map(start, (uint64_t)start, len, pte_flags);
}

static unsigned uefi_attr_to_pte_flags(uint64_t uefi_attr)
{
	if ((uefi_attr & EFI_MEMORY_WB) != 0)
		return 0;
	else if ((uefi_attr & EFI_MEMORY_WT) != 0)
		return PTE_WT;
	else
		return PTE_CD;
}

static void va_init(void)
{
	unsigned nvr, mvr, i;
	/*
	 * First allocate some memory to keep track of unused virtual
	 * memory address ranges, & initialize these.
	 */
	mvr = max_mem_ranges;
	unused_va_ranges = mem_alloc(mvr * sizeof(va_range_t),
				     _Alignof(va_range_t), 0);
	nvr = 0;
	for (i = 1; i <= num_mem_ranges; ++i) {
		uint64_t prev_end, start;
		prev_end = mem_ranges[i - 1].start + mem_ranges[i - 1].len;
		if (prev_end - 1 >= XM32_MAX_ADDR - 1)
			break;
		prev_end = (prev_end + PAGE_SIZE - 1) & -(uint64_t)PAGE_SIZE;
		if (prev_end - 1 >= XM32_MAX_ADDR - 1)
			break;
		/*
		 * As a special case, skip over any "holes" that occur in
		 * base memory (< 1 MiB).  These may not actually be holes.
		 */
		if (prev_end < BMEM_MAX_ADDR)
			prev_end = BMEM_MAX_ADDR;
		if (i == num_mem_ranges)
			start = XM32_MAX_ADDR;
		else {
			start = mem_ranges[i].start & -(uint64_t)PAGE_SIZE;
			if (start > XM32_MAX_ADDR)
				start = XM32_MAX_ADDR;
		}
		if (prev_end >= start)
			continue;
		unused_va_ranges[nvr].start = (uint32_t)prev_end;
		unused_va_ranges[nvr].len =
		    (uint32_t)start - (uint32_t)prev_end;
		++nvr;
	}
	num_unused_va_ranges = nvr;
	max_unused_va_ranges = mvr;
	/*
	 * Set up the page-directory-pointer table (PDPT) & the 4 page
	 * directories (PDs) for PAE paging.
	 */
	pdpt = mem_alloc(4 * sizeof(uint64_t), PDPT_ALIGN, 0);
	for (i = 0; i < 4; ++i) {
		uint64_t *pd = mem_alloc(PAGE_SIZE, PAGE_SIZE, 0);
		memset(pd, 0, PAGE_SIZE);
		pdpt[i] = (uint64_t)(uint32_t)pd | PTE_P;
	}
	/*
	 * Fill in the page directories & page tables (PTs) with identity
	 * mappings for all physical memory addresses below 4 GiB that may
	 * be backed by physical hardware.
	 */
	do_va_id_map(0, unused_va_ranges[0].start,
	    uefi_attr_to_pte_flags(mem_ranges[0].uefi_attr));
	for (i = 1; i < nvr; ++i) {
		uint32_t prev_end = unused_va_ranges[i - 1].start +
				    unused_va_ranges[i - 1].len;
		uint32_t start = unused_va_ranges[i].start;
		do_va_id_map(prev_end, start - prev_end, 0);
	}
	/*
	 * If there are any memory ranges that are non-cacheable or only
	 * write-through cacheable, then modify the PTs to properly handle
	 * these.
	 *
	 * Make the pages write-through where write-through is supported.
	 * Make the rest of the pages uncached.
	 *
	 * FIXME: the mem_ranges[] array may expand in the course of this
	 * loop.  This should not affect correctness though.
	 */
	i = 0;
	while (i < num_mem_ranges) {
		mem_range_t *mr = &mem_ranges[i];
		uint64_t start64, len64;
		uint32_t start, len;
		unsigned pte_flags = uefi_attr_to_pte_flags(mr->uefi_attr);
		if (!pte_flags) {
			++i;
			continue;
		}
		start64 = mem_ranges[i].start;
		if (start64 >= XM32_MAX_ADDR) {
			++i;
			continue;
		}
		start = (uint32_t)start64;
		len64 = mem_ranges[i].len;
		if (len64 >= XM32_MAX_ADDR - start)
			len = (uint32_t)XM32_MAX_ADDR - start;
		else
			len = (uint32_t)len64;
		if (start % PAGE_SIZE != 0) {
			len += start % PAGE_SIZE;
			start &= -PAGE_SIZE;
		}
		if (len % PAGE_SIZE != 0)
			len = (len + PAGE_SIZE - 1) & -PAGE_SIZE;
		do_va_id_map(start, len, pte_flags);
		while (i < num_mem_ranges && mem_ranges[i].start != start64)
			++i;
	}
	/* Bring up our page tables. */
	wr_cr4(rd_cr4() | CR4_PAE);
	wr_cr3((uint32_t)pdpt);
	wr_cr0(rd_cr0() | CR0_PG);
}

/* Initialize memory allocation & virtual memory addressing. */
void mem_init(bparm_t *bparms)
{
	mem_map_init(bparms);
	va_init();
}

/*
 * Reserve some physical memory for internal use.  If `max_addr' != 0, the
 * end of the memory block will be below `max_addr'.
 */
void *mem_alloc(size_t sz, size_t align, uintptr_t max_addr)
{
	uint64_t max_addr64;
	uintptr_t astart, aend;
	mem_range_t *mr;
	if (!sz)
		return NULL;
	max_addr64 = max_addr;
	if (!max_addr)
		max_addr64 = XM32_MAX_ADDR;
	if (max_addr64 < sz)
		hlt();
	for (mr = find_highest_range_below(max_addr64); ; --mr) {
		if (mr->e820_type != E820_RAM || mr->len < sz)
			continue;
		aend = mr->start + mr->len;
		astart = (aend - sz) & -align;
		if (astart >= mr->start)
			break;
		if (!mr->start)
			hlt();
	}
	split_range(mr, astart, E820_RAM, E820_RESERVED);
	return (void *)astart;
}

/*
 * Map some physical memory --- possibly beyond the 32-bit physical space
 * --- into our 32-bit virtual address space.
 */
void *mem_do_va_map(uint64_t pa, size_t sz, unsigned pte_flags)
{
	uint64_t pstart, pend, sz_to_map;
	uint32_t vstart;
	unsigned i;
	if (!sz)
		return NULL;
	pstart = pa & -(uint64_t)PAGE_SIZE;
	pend = (pa + sz + PAGE_SIZE - 1) & -(uint64_t)PAGE_SIZE;
	sz_to_map = pend - pstart;
	if (sz_to_map >= XM32_MAX_ADDR)
		hlt();
	i = num_unused_va_ranges;
	while (i-- != 0) {
		uint32_t vrsz = unused_va_ranges[i].len;
		if (vrsz >= sz) {
			vrsz -= sz;
			unused_va_ranges[i].len = vrsz;
			vstart = unused_va_ranges[i].start + vrsz;
			do_va_map(vstart, pstart, sz_to_map, pte_flags);
			return (char *)vstart + (size_t)(pa % PAGE_SIZE);
		}
	}
	hlt();
	__builtin_unreachable();
}
