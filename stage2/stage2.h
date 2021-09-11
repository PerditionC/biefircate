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

/* irq.c functions. */

extern void irq_init(bparm_t *);

/* mem.c functions. */

extern void mem_init(bparm_t *);
extern void *mem_alloc(size_t, size_t, uintptr_t);
extern void *mem_va_map(uint64_t, size_t, unsigned);
extern void mem_va_unmap(volatile void *, size_t);

/* rm16.asm functions. */

extern uint16_t rm16_cs;
extern void rm16_init(void);
extern void rm16_call(uint32_t eax, uint32_t edx, uint32_t ecx, uint32_t ebx,
		      farptr16_t callee);

/* SeaBIOSify functions. */

extern void extra_stack_init(void);
extern void ps2_keyboard_setup(void *);

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

/* Flags in the ... flags register. */
#define EFL_C		(1U <<  0)	/* carry */
#define EFL_P		(1U <<  2)	/* parity */
#define EFL_A		(1U <<  4)	/* auxiliary carry */
#define EFL_Z		(1U <<  6)	/* zero */
#define EFL_S		(1U <<  7)	/* sign */
#define EFL_T		(1U <<  8)	/* trace */
#define EFL_I		(1U <<  9)	/* interrupt */
#define EFL_D		(1U << 10)	/* direction */
#define EFL_O		(1U << 11)	/* overflow */

/* Address spaces when in 16-bit real mode. */
#define LOWDATA		__seg_fs	/* our 16-bit data segment */

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

/*
 * Data structure for registers passed to & from a 16-bit interrupt service
 * routine.
 */
typedef struct __attribute__((packed)) {
	uint16_t gs;
	uint16_t fs;
	uint16_t ds;
	uint16_t es;
	union {
		uint32_t edi;
		uint16_t di;
	};
	union {
		uint32_t esi;
		uint16_t si;
	};
	union {
		uint32_t ebp;
		uint16_t bp;
	};
	union {
		uint32_t ebx;
		uint16_t bx;
		struct { uint8_t bl, bh; };
	};
	union {
		uint32_t edx;
		uint16_t dx;
		struct { uint8_t dl, dh; };
	};
	union {
		uint32_t ecx;
		uint16_t cx;
		struct { uint8_t cl, ch; };
	};
	union {
		uint32_t eax;
		uint16_t ax;
		struct { uint8_t al, ah; };
	};
	farptr16_t caller;
	uint16_t flags;
} isr16_regs_t;

/* Fashion a far 16-bit pointer from a 16-bit segment & a 16-bit offset. */
static inline farptr16_t MK_FP16(uint16_t seg, uint16_t off)
{
	return (farptr16_t)seg << 16 | off;
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

/* Read cr3. */
static inline uint32_t rd_cr3(void)
{
	uint32_t v;
	__asm volatile("movl %%cr3, %0" : "=r" (v));
	return v;
}

/* Write cr3. */
static inline void wr_cr3(uint32_t v)
{
	__asm volatile("movl %0, %%cr3" : : "r" (v) : "memory");
}

/* Flush page table caches by reading & writing cr3. */
static inline void flush_cr3(void)
{
	wr_cr3(rd_cr3());
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

/* Write a byte to an I/O port. */
static inline void outp(uint16_t p, uint8_t v)
{
	__asm volatile("outb %1, %0" : : "Nd" (p), "a" (v));
}

/* Write a byte to an I/O port, then add a small wait. */
static inline void outp_w(uint16_t p, uint8_t v)
{
	outp(p, v);
	__asm volatile("outb %%al, %0" : : "Nd" ((uint16_t)0x80));
}

#endif
