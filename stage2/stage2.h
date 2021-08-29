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

/* Legacy 8259 programmable interrupt controller (PIC) I/O port numbers. */
#define PIC1_CMD	0x0020
#define PIC1_DATA	0x0021
#define PIC2_CMD	0x00a0
#define PIC2_DATA	0x00a1

/* OCW2 bit fields for the PICs. */
#define OCW2_EOI	0x20		/* non-specific EOI */

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

/* BIOS data area variables. */
typedef struct __attribute__((packed)) {
	uint16_t com1;			/* 0x40:0x00: 1st serial port base */
	uint16_t com2;			/* 0x40:0x02: 2nd serial port base */
	uint16_t com3;			/* 0x40:0x04: 3rd serial port base */
	uint16_t com4;			/* 0x40:0x06: 4th serial port base */
	uint16_t lpt1;			/* 0x40:0x08: 1st parallel port base */
	uint16_t lpt2;			/* 0x40:0x0a: 2nd parallel port base */
	uint16_t lpt3;			/* 0x40:0x0c: 3rd parallel port base */
	uint16_t ebda;			/* 0x40:0x0e: ext. BIOS data area */
	uint16_t eqpt;			/* 0x40:0x10: installed hardware */
	uint8_t : 8;
	uint16_t base_kib;		/* 0x40:0x13: base mem. size in KiB */
	uint16_t : 16;
	uint8_t kb_stat1;		/* 0x40:0x17: status flag 1 */
	uint8_t kb_stat2;		/* 0x40:0x18: status flag 2 */
	uint8_t kb_keypad;		/* 0x40:0x19: Alt-nnn keypad worksp. */
	uint16_t kb_buf_head;		/* 0x40:0x1a: head in kb. buf. queue */
	uint16_t kb_buf_tail;		/* 0x40:0x1c: tail in kb. buf. queue */
	uint16_t kb_buf[16];		/* 0x40:0x1e: default kb. buffer */
	uint8_t fd_recalib;		/* 0x40:0x3e: floppy recalib. stat. */
	uint8_t fd_motor;		/* 0x40:0x3f: floppy motor status */
	uint8_t fd_cntdn;		/* 0x40:0x40: floppy motor turn-off
						      timeout count */
	uint8_t fd_error;		/* 0x40:0x41: floppy last op. status */
	uint8_t dsk_status[7];		/* 0x40:0x42: floppy/hard drive status/
						      command bytes */
	uint8_t vid_mode;		/* 0x40:0x49: video mode */
	uint16_t vid_cols;		/* 0x40:0x4a: columns on screen */
	uint16_t vid_page_sz;		/* 0x40:0x4c: video page size */
	uint16_t vid_page_start;	/* 0x40:0x4e: video page start addr. */
	uint16_t vid_xy[8];		/* 0x40:0x50: cursor position in each
						      of 8 video pages */
	uint16_t vid_curs_shape;	/* 0x40:0x60: cursor shape */
	uint8_t vid_pg;			/* 0x40:0x62: video page no. */
	uint16_t crtc;			/* 0x40:0x63: CRT base I/O port addr.
						      (normally 0x03b4 or
						      0x03d4) */
	uint8_t vid_msr;		/* 0x40:0x65: last value written to
						      port 0x03b8 or 0x03d8 */
	uint8_t vid_pal;		/* 0x40:0x66: last value written to
						      port 0x03d9 */
	farptr16_t restart;		/* 0x40:0x67: reset restart addr. */
	uint8_t stray_irq;		/* 0x40:0x6b: IRQ in-service reg.
						      value from time of last
						      stray IRQ */
	uint32_t timer;			/* 0x40:0x6c: timer ticks since
						      midnight */
	uint8_t timer_ovf;		/* 0x40:0x70: timer overflow */
	uint8_t ctrlc;			/* 0x40:0x71: Ctrl-Break flag */
	uint16_t reset_flag;		/* 0x40:0x72: POST reset flag */
	uint8_t hd_error;		/* 0x40:0x74: fixed disk last op.
						      status */
	uint8_t hd_cnt;			/* 0x40:0x75: no. of fixed disk
						      drives */
	uint8_t hd_ctl;			/* 0x40:0x76: fixed disk ctrl. byte */
	uint8_t hd_port_off;		/* 0x40:0x77: fixed disk I/O port
						      offset */
	uint8_t lpt1_cntdn;		/* 0x40:0x78: par. dev. 1 timeout */
	uint8_t lpt2_cntdn;		/* 0x40:0x79: par. dev. 2 timeout */
	uint8_t lpt3_cntdn;		/* 0x40:0x79: par. dev. 3 timeout */
	uint8_t flags_0x4b;		/* 0x40:0x7b: int 0x4b flags */
	uint8_t com1_cntdn;		/* 0x40:0x7c: ser. dev. 1 timeout */
	uint8_t com2_cntdn;		/* 0x40:0x7d: ser. dev. 2 timeout */
	uint8_t com3_cntdn;		/* 0x40:0x7e: ser. dev. 3 timeout */
	uint8_t com4_cntdn;		/* 0x40:0x7f: ser. dev. 4 timeout */
	uint16_t kb_buf_start;		/* 0x40:0x80: kbd. buf. start offset */
	uint16_t kb_buf_end;		/* 0x40:0x82: kbd. buf. end offset */
} bda_t;

extern __seg_gs bda_t bda;

/*
 * Real-mode segment of the BIOS data area.  BDA_SEG:0 refers to the same
 * address as the `bda' structure.
 */
#define BDA_SEG		0x40

#define DATA16		__seg_fs

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

/* Read a byte from an I/O port. */
static inline uint8_t inp(uint16_t p)
{
	uint8_t v;
	__asm volatile("inb %1, %0" : "=a" (v) : "Nd" (p));
	return v;
}

/* Read a byte from an I/O port, with a small wait. */
static inline uint8_t inp_w(uint16_t p)
{
	__asm volatile("outb %%al, %0" : : "Nd" ((uint16_t)0x80));
	return inp(p);
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

/* Disable interrupts. */
static inline void cli(void)
{
	__asm volatile("cli" : : : "memory");
}

/* Enable interrupts. */
static inline void sti(void)
{
	__asm volatile("sti" : : : "memory");
}

/* Write a shortword at a segment:offset address. */
static inline void poke(uint16_t s, uint32_t o, uint16_t v)
{
	uint16_t scratch;
	__asm volatile(	"movw %%ds, %0; "
			"movw %1, %%ds; "
			"movw %3, %a2; "
			"movw %0, %%ds"
	    : "=&g" (scratch)
	    : "rm" (s), "p" (o), "r" (v)
	    : "memory");
}

#endif
