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

/* Routines for passing boot parameters to the stage 2 bootloader. */

#include <stdlib.h>
#include <string.h>
#include "stage1/stage1.h"

static bparm_t *bp_head = NULL, *bp_tail = NULL;

/*
 * Add a boot parameter node with the given type & a data field of the given
 * size.  Return a pointer to the data field, which the caller should then
 * fill with the actual data.
 */
void *bparm_add(uint32_t type, uint32_t size)
{
	bparm_t *bp = bmem_alloc_boottime(sizeof(bparm_t) + size,
					  sizeof(void *));
	if (!bp_head)
		bp_head = bp_tail = bp;
	else {
		bp_tail->next = bp;
		bp_tail = bp;
	}
	bp->next = NULL;
	bp->type = type;
	bp->size = size;
	memset(bp->u, 0, size);
	return bp->u;
}

/*
 * Convenience function: add a boot parameter node for a memory address
 * range.  If the range is empty, do nothing.
 */
bdat_mem_range_t *bparm_add_mem_range(uint64_t start, uint64_t len,
    uint32_t e820_type, uint32_t e820_ext_attr, uint64_t uefi_attr)
{
	bdat_mem_range_t *bd;
	if (!len)
		return NULL;
	bd = bparm_add(BP_MRNG, sizeof(bdat_mem_range_t));
	bd->start = start;
	bd->len = len;
	bd->e820_type = e820_type;
	bd->e820_ext_attr = e820_ext_attr;
	bd->uefi_attr = uefi_attr;
	return bd;
}

/* Return the linked list of boot parameters built up. */
bparm_t *bparm_get(void)
{
	return bp_head;
}
