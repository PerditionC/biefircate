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
#include "stage2/stage2.h"

static unsigned num_mem_ranges = 0, max_mem_ranges = 0;
static mem_range_t *mem_ranges;

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

/* Initialize memory allocation. */
void mem_init(bparm_t *bparms)
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
		if (bp->type == BP_MRNG)
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
