/*
 * Copyright (c) 2020 TK Chia
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

/*
 * Routines for maintaining a heap for dynamic memory allocation. 
 * Currently, these just allocate a fixed number of contiguous pages of
 * memory at startup, & later parcel out slices of memory from these pages
 * as needed.  This scheme should be good enough for now.
 */

#include <efi.h>
#include <efilib.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "efi-stuff.h"
#include "truckload.h"

#define DIV_ROUND_UP(x, y)	(((x) + (y) - 1) / (y))
#define ALLOC_QUANTUM		(4 * __BIGGEST_ALIGNMENT__)
#if EFI_PAGE_SIZE % ALLOC_QUANTUM != 0
#   error "dynamic memory allocation quantum does not align with page size...?"
#endif
#define HEAP_PAGES		DIV_ROUND_UP(0x80000, EFI_PAGE_SIZE)
#define HEAP_BYTES		(HEAP_PAGES * EFI_PAGE_SIZE)
#define HEAP_QUANTA		(HEAP_BYTES / ALLOC_QUANTUM)
#if HEAP_BYTES / ALLOC_QUANTUM <= 0x7fULL
typedef int8_t metadata_t;
#elif HEAP_BYTES / ALLOC_QUANTUM  <= 0x7fffULL
typedef int16_t metadata_t;
#elif HEAP_BYTES / ALLOC_QUANTUM  <= 0x7fffffffULL
typedef int32_t metadata_t;
#else
typedef int64_t metadata_t;
#endif

static EFI_PHYSICAL_ADDRESS heap_start = 0;
/*
 * Information about heap allocations goes here:
 *   * metadata[i] < 0: heap_start + i * HEAP_QUANTA starts an allocated heap
 *     chunk, absolute value gives chunk size (in quanta)
 *   * metadata[i] == 0: heap_start + i * HEAP_QUANTA is in middle of a chunk
 *   * metadata[i] > 0: heap_start + i * HEAP_QUANTA starts a free heap chunk
 *     chunk, value gives chunk size.
 * The last array element is a sentinel which is always 0.
 */
static metadata_t metadata[HEAP_QUANTA + 1];

static void mem_alloc_fail(size_t size)
{
	panic("%s: failed to allocate %#zx-byte block on heap",
	    __func__, size);
}

static void check_chunk_and_get_metadata(void *p, metadata_t *ip,
						  metadata_t *dp)
{
	EFI_PHYSICAL_ADDRESS a = (EFI_PHYSICAL_ADDRESS)p;
	metadata_t i, d;
	if (a < heap_start || a >= heap_start + HEAP_BYTES ||
	    a % ALLOC_QUANTUM != 0)
		panic("%s: bogus heap block address @%p", __func__, p);
	i = (a - heap_start) / ALLOC_QUANTUM;
	d = metadata[i];
	if (d >= 0)
		panic("%s: bogus heap block address @%p, "
		      "metadata says %d", __func__, a, (int)d);
	if (metadata[HEAP_QUANTA] != 0)
		panic("%s: heap metadata sentinel destroyed: %d",
		    __func__, (int)metadata[HEAP_QUANTA]);
	*ip = i;
	*dp = d;
}

static metadata_t coalesce(metadata_t i)
{
	metadata_t d = metadata[i];
	while (d > 0 && metadata[i + d] > 0) {
		metadata_t nd = d + metadata[i + d];
		metadata[i + d] = 0;
		metadata[i] = d = nd;
	}
	return d;
}

INIT_TEXT void mem_heap_init(void)
{
	EFI_STATUS status = BS->AllocatePages(AllocateAnyPages, EfiLoaderData,
	    HEAP_PAGES, &heap_start);
	if (EFI_ERROR(status))
		panic_efi("cannot reserve memory for internal heap", status);
	cprintf("using @%p--@%p for internal heap\n",
	    (void *)heap_start, (void *)(heap_start + HEAP_BYTES - 1));
	memset(metadata, 0, sizeof metadata);
	metadata[0] = HEAP_QUANTA;
}

void *mem_heap_alloc(size_t size)
{
	metadata_t q, i;
	if (size > HEAP_BYTES)
		mem_alloc_fail(size);
	if (!size)
		size = 1;
	q = DIV_ROUND_UP(size, ALLOC_QUANTUM);
	i = 0;
	while (i <= HEAP_QUANTA - q) {
		metadata_t d = coalesce(i);
		if (d >= q) {
			if (d != q) {
				/* Split chunk into two. */
				metadata[i + q] = d - q;
			}
			metadata[i] = -q;
			return (void *)(heap_start +
					   (size_t)i * ALLOC_QUANTUM);
		}
		if (d > 0)
			i += d;
		else if (d < 0)
			i -= d;
		else {
			cprintf("heap metadata element %u is 0?",
			    (unsigned)i);
			panic("heap corrupt!");
		}
	}
	mem_alloc_fail(size);
	__builtin_unreachable();
}

void mem_heap_free(void *p)
{
	metadata_t i, d;
	if (!p)
		return;
	check_chunk_and_get_metadata(p, &i, &d);
	metadata[i] = -d;
}

void *mem_heap_realloc(void *p, size_t size)
{
	metadata_t i, d, oq, q;
	void *p2;
	if (!p)
		return mem_heap_alloc(size);
	if (!size)
		size = 1;
	check_chunk_and_get_metadata(p, &i, &oq);
	q = DIV_ROUND_UP(size, ALLOC_QUANTUM);
	/* Compute original allocation size. */
	oq = -oq;
	if (oq == q) {
		/* Reallocating to same size --- nothing to do. */
		return p;
	}
	if (oq > q) {
		/* Shrinking.  Create a new free chunk. */
		metadata[i + q] = oq - q;
		metadata[i] = -q;
		return p;
	}
	/*
	 * Expanding.  If the original chunk is followed by free chunks of
	 * sufficient size...
	 */
	d = coalesce(i + oq);
	if (d >= q - oq) {
		if (d != q - oq)
			metadata[i + q] = d - (q - oq);
		metadata[i] = -q;
		metadata[i + oq] = 0;
		return p;
	}
	/* If not... */
	p2 = mem_heap_alloc(size);
	memcpy(p2, p, (size_t)oq * ALLOC_QUANTUM);
	mem_heap_free(p);
	return p2;
}
