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

#ifndef H_STAGE2_STAGE2
#define H_STAGE2_STAGE2

#include <inttypes.h>
#include <stdlib.h>
#include "bparm.h"

typedef uint32_t farptr16_t;

/* mem.c functions. */

extern void mem_init(bparm_t *);
extern void *mem_alloc(size_t, size_t, uintptr_t);
extern void *mem_va_map(uint64_t, size_t);

/* rm16.asm functions. */

extern uint16_t rm16_cs;
extern void rm16_init(void);
extern void rm16_call(uint32_t eax, uint32_t edx, uint32_t ecx, uint32_t ebx,
		      farptr16_t callee);

/* Macros, inline functions, & other definitions. */

#define XM32_MAX_ADDR	0x100000000ULL	/* end of 32-bit extended memory,
					   i.e. the 4 GiB mark */
#define PAGE_SIZE	0x1000UL	/* size of a virtual memory page */
#define LARGE_PAGE_SIZE	0x200000UL	/* size of a larger VM page */
#define PDPT_ALIGN	0x20U		/* alignment of the page-dir.-ptr.
					   table (PDPT) for PAE paging */

/* Flags in PAE page directory & page table entries. */
#define PTE_P		(1UL <<  0)	/* present */
#define PTE_RW		(1UL <<  1)	/* read/write */
#define PTE_US		(1UL <<  2)	/* user/supervisor */
#define PTE_WT		(1UL <<  3)	/* write-through */
#define PTE_CD		(1UL <<  4)	/* cache disable */
#define PDE_PS		(1UL <<  7)	/* (page dir.) large page size */

/* Flags in the cr0 register. */
#define CR0_PG		(1UL << 31)	/* paging */
#define CR0_PE		(1UL <<  0)	/* protection enable */

/* Flags in the cr4 register. */
#define CR4_PAE		(1UL <<  5)	/* physical address extension (PAE) */

/*
 * Data structure describing a single memory address range.  The front part
 * is in the same format as returned by int 0x15, ax = 0xe820.
 */
typedef struct __attribute__((packed)) mem_range {
	uint64_t start;
	uint64_t len;
	uint32_t e820_type;
	uint32_t e820_ext_attr;
	uint64_t uefi_attr;
} mem_range_t;

/* Fashion a far 16-bit pointer from a 16-bit segment & a 16-bit offset. */
static inline uint32_t MK_FP16(uint16_t seg, uint16_t off)
{
	return (uint32_t)seg << 16 | off;
}

/* Read cr0. */
static inline uint32_t rd_cr0(void)
{
	uint32_t v;
	__asm volatile("movl %%cr0, %0" : "=r" (v));
	return v;
}

/* Write cr0. */
static inline void wr_cr0(uint32_t v)
{
	__asm volatile("movl %0, %%cr0" : : "r" (v) : "memory");
}

/* Write cr3. */
static inline void wr_cr3(uint32_t v)
{
	__asm volatile("movl %0, %%cr3" : : "r" (v) : "memory");
}

/* Read cr4. */
static inline uint32_t rd_cr4(void)
{
	uint32_t v;
	__asm volatile("movl %%cr4, %0" : "=r" (v));
	return v;
}

/* Write cr4. */
static inline void wr_cr4(uint32_t v)
{
	__asm volatile("movl %0, %%cr4" : : "r" (v) : "memory");
}

#endif
