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

#include <stdbool.h>
#include <string.h>
#include "stage1/stage1.h"

/*
 * Base memory below the 1 MiB mark --- especially the base memory starting
 * at physical address 0 --- needs to be handled specially.
 *
 * We want to keep as much of this base memory free as possible, for running
 * 16-bit real mode code.  More importantly, we do not want the UEFI runtime
 * to suddenly allocate pages from the base memory area after we have
 * determined its size.
 *
 * To ensure this, bmem_init() first reserves as much base memory as
 * possible.  Any attempts to allocate from base memory should then go
 * through this module instead of going directly to UEFI.
 */

#define BMEM_MAX_ADDR	0x100000ULL
#define MAX_BMEM_BLKS	(BMEM_MAX_ADDR / EFI_PAGE_SIZE / 2)

typedef struct {
	UINT32 start, end;
} blk_info_t;

/* Variables to keep track of available base memory blocks. */
static UINTN num_blks = 0;
static blk_info_t blk[MAX_BMEM_BLKS];
/*
 * Variable to keep track of the size of the base memory starting at address
 * 0 that will be available at run time.
 */
static UINT32 runtime_bmem_top = 0;

/* Initialize base memory allocation. */
void bmem_init(void)
{
	UINTN page_count = BMEM_MAX_ADDR / EFI_PAGE_SIZE / 2;
	EFI_MEMORY_DESCRIPTOR *desc, *descs;
	EFI_PHYSICAL_ADDRESS start, end;
	UINT32 idx, end_idx;
	UINTN num_ents, map_key, desc_sz, ent_iter, num_extra_blks = 0;
	/* Bit vector saying whether each page is available for use. */
	BVEC_TYPE(BMEM_MAX_ADDR / EFI_PAGE_SIZE) avail;
	memset(&avail, 0, sizeof avail);
	/*
	 * Grab all the base memory pages we can grab from UEFI.  Mark as
	 * available all the pages we can grab at boot time.
	 */
	do {
		for (;;) {
			start = BMEM_MAX_ADDR;
			EFI_STATUS status =
			    BS->AllocatePages(AllocateMaxAddress,
				EfiRuntimeServicesData, page_count, &start);
			if (EFI_ERROR(status))
				break;
			idx = (UINT32)(start / EFI_PAGE_SIZE);
			while (page_count-- != 0) {
				bvec_set(&avail, idx);
				++idx;
			}
		}
		page_count /= 2;
	} while (page_count);
	/* Group the available base memory into blocks for ease of tracking. */
	Output(u"avail. base mem. blocks:");
	start = BMEM_MAX_ADDR;
	for (idx = 0; idx < BMEM_MAX_ADDR / EFI_PAGE_SIZE; ++idx) {
		if (bvec_test(&avail, idx)) {
			if (start == BMEM_MAX_ADDR)
				start = (UINT32)idx * EFI_PAGE_SIZE;
		} else if (start != BMEM_MAX_ADDR) {
			end = (UINT32)idx * EFI_PAGE_SIZE;
			blk[num_blks].start = start;
			blk[num_blks].end = end;
			if (num_blks % 4 == 0)
				Output(u"\r\n");
			Print(u"  @0x%lx~@0x%lx", start, end - 1);
			++num_blks;
			start = BMEM_MAX_ADDR;
		}
	}
	if (start != BMEM_MAX_ADDR) {
		end = (UINT32)idx * EFI_PAGE_SIZE;
		blk[num_blks].start = start;
		blk[num_blks].end = end;
		if (num_blks % 4 == 0)
			Output(u"\r\n");
		Print(u"  @0x%lx~@0x%lx", start, end - 1);
		++num_blks;
	}
	/*
	 * While at it, also start to gauge the base memory at 0 that will
	 * be available at run time (for filling in 0x40:0x13 later).  For
	 * this, we should also count existing EfiBootServices{Code, Data} &
	 * EfiLoader{Code, Data} pages, which will effectively be freed once
	 * we exit boot services.  To count this, we need to obtain a UEFI
	 * memory map.
	 */
	descs = get_mem_map(&num_ents, &map_key, &desc_sz);
	FOR_EACH_MEM_DESC(desc, descs, desc_sz, num_ents, ent_iter) {
		switch (desc->Type) {
		    default:
			break;
		    case EfiLoaderCode:
		    case EfiLoaderData:
		    case EfiBootServicesCode:
		    case EfiBootServicesData:
			start = desc->PhysicalStart;
			if (start >= BMEM_MAX_ADDR)
				break;
			idx = start / EFI_PAGE_SIZE;
			end_idx = idx + desc->NumberOfPages;
			end = end_idx * EFI_PAGE_SIZE;
			if (end_idx > BMEM_MAX_ADDR / EFI_PAGE_SIZE)
				end_idx = BMEM_MAX_ADDR / EFI_PAGE_SIZE;
			if ((num_blks + num_extra_blks) % 4 == 0)
				Output(u"\r\n");
			Print(u"  [@0x%lx~@0x%lx]", start, end - 1);
			++num_extra_blks;
			while (idx < end_idx) {
				bvec_set(&avail, idx);
				++idx;
			}
		}
	}
	Output(u"\r\n");
	idx = 0;
	while (idx < BMEM_MAX_ADDR / EFI_PAGE_SIZE && bvec_test(&avail, idx))
		++idx;
	if (idx < (192 * KIBYTE) / EFI_PAGE_SIZE)
		error(u"not enough base mem.!");
	runtime_bmem_top = (UINT32)idx * EFI_PAGE_SIZE;
}

/*
 * Allocate base memory.  `align' gives the requested alignment, which should
 * be a power of 2.
 */
void *bmem_alloc(UINTN size, UINTN align)
{
	UINTN blk_idx;
	/* Try to allocate from higher-addressed blocks first. */
	blk_idx = num_blks;
	while (blk_idx-- != 0) {
		UINT32 bstart = blk[blk_idx].start, bend = blk[blk_idx].end;
		UINT32 astart;
		if (bend - bstart < size)
			continue;
		astart = (bend - size) & -(UINT32)align;
		if (astart < bstart)
			continue;
		/* Success! */
		if (runtime_bmem_top > astart)
			runtime_bmem_top = astart;
		if (astart != bstart)
			blk[blk_idx].end = astart;
		else {
			while (blk_idx < num_blks - 1) {
				blk[blk_idx] = blk[blk_idx + 1];
				++blk_idx;
			}
			--num_blks;
		}
		return (void *)(EFI_PHYSICAL_ADDRESS)astart;
	}
	error(u"cannot alloc. from base mem.!");
}

/* Wrap up base memory allocation. */
unsigned bmem_fini(void)
{
	UINT32 top = runtime_bmem_top & -KIBYTE;
	Print(u"base mem. blk. @0 ends at @0x%x\r\n", top);
	return top / KIBYTE;
}
