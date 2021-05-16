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

#ifndef H_TRUCKLOAD
#define H_TRUCKLOAD

/* Interrupt stack indices. */
#define INT_STACK_EXCP		1	/* for most exceptions */
#define INT_STACK_BAD_EXCP	2	/* for really bad exceptions (NMI,
					   double fault) */
#define INT_STACK_IRQ		3	/* for IRQs */

#ifndef __ASSEMBLER__
#   include <inttypes.h>
#   include <stdarg.h>
#   include <stdbool.h>
#   include <stdlib.h>
#   include <uchar.h>

/* Shorthands for C function & data attributes. */
#   define INIT_TEXT	__attribute__((section(".text.i")))
#   define INIT_DATA	__attribute__((section(".data.i")))
#   define NORETURN	__attribute__((noreturn))
#   define PURE		__attribute__((pure))

#   define PAGE_SIZE	0x1000ULL
#   define LARGE_PAGE_SIZE (0x200ULL * PAGE_SIZE)

enum COLORS
{
	BLACK, BLUE, GREEN, CYAN,
	RED, MAGENTA, BROWN, LIGHTGRAY,
	DARKGRAY, LIGHTBLUE, LIGHTGREEN, LIGHTCYAN,
	LIGHTRED, LIGHTMAGENTA, YELLOW, WHITE
};

static inline void disable(void)
{
	__asm volatile("cli");
}

static inline void enable(void)
{
	__asm volatile("sti");
}

static inline NORETURN void freeze(void)
{
	__asm volatile("cli; "
		       "0: "
		       "hlt; "
		       "jmp 0b");
	__builtin_unreachable();
}

static inline PURE uint16_t get_cs(void)
{
	uint16_t cs_val;
	__asm volatile("movw %%cs, %0" : "=rm" (cs_val));
	return cs_val;
}

/*
 * GNU EFI defines macros inp, inpw, inpd, outp, outpw, & outpd which go
 * through UEFI & conflict with our definitions.
 */
#undef inp
#undef inpw
#undef inpd
#undef outp
#undef outpw
#undef outpd

static inline uint8_t inp(uint16_t port)
{
	uint8_t value;
	__asm volatile("inb %1, %0" : "=a" (value) : "Nd" (port));
	return value;
}

static inline uint16_t inpw(uint16_t port)
{
	uint16_t value;
	__asm volatile("inw %1, %0" : "=a" (value) : "Nd" (port));
	return value;
}

static inline uint32_t inpd(uint16_t port)
{
	uint32_t value;
	__asm volatile("inl %1, %0" : "=a" (value) : "Nd" (port));
	return value;
}

static inline void outp(uint16_t port, uint8_t value)
{
	__asm volatile("outb %1, %0" : : "Nd" (port), "a" (value));
}

static inline void outpw(uint16_t port, uint16_t value)
{
	__asm volatile("outw %1, %0" : : "Nd" (port), "a" (value));
}

static inline void outpd(uint16_t port, uint32_t value)
{
	__asm volatile("outl %1, %0" : : "Nd" (port), "a" (value));
}

typedef struct {
	union {
		uint32_t a;
		uint8_t a_bytes[4];
	};
	union {
		uint32_t b;
		uint8_t b_bytes[4];
	};
	union {
		uint32_t c;
		uint8_t c_bytes[4];
	};
	union {
		uint32_t d;
		uint8_t d_bytes[4];
	};
} cpuid_t;

static inline cpuid_t cpuid(uint32_t leaf)
{
	cpuid_t id;
	__asm volatile("cpuid"
		       : "=a" (id.a), "=b" (id.b), "=c" (id.c), "=d" (id.d)
		       : "0" (leaf));
	return id;
}

static inline cpuid_t cpuid_with_subleaf(uint32_t leaf, uint32_t subleaf)
{
	cpuid_t id;
	__asm volatile("cpuid"
		       : "=a" (id.a), "=b" (id.b), "=c" (id.c), "=d" (id.d)
		       : "0" (leaf), "2" (subleaf));
	return id;
}

#define assert(p)	((p) || (assert_fail(#p), 0))

/* acpi.c */
extern INIT_TEXT void acpi_init(const void *);

/* apic.c */
extern INIT_TEXT void apic_init(uintptr_t, int_fast16_t, bool, bool,
    bool, unsigned);
extern INIT_TEXT void apic_add_ioapic(uint8_t, uintptr_t, uint32_t);
extern INIT_TEXT void apic_add_ioapic_nmi(uint32_t, bool, bool);
extern INIT_TEXT void apic_add_irq_override(uint8_t, uint8_t, uint32_t,
    bool, bool);

/* cpuid.c */
extern INIT_TEXT void cpuid_init(void);
extern INIT_TEXT bool cpuid_has_std_leaf_p(uint32_t);

/* fb-con.c */
extern INIT_TEXT void fb_con_init(void);
extern INIT_TEXT uintptr_t fb_con_mem_end(void);
extern void putwch(char16_t);
extern void cputws(const char16_t *);
extern void cnputs(const char *, size_t);
extern void cputs(const char *);
extern enum COLORS textcolor(enum COLORS);
extern enum COLORS warnvideo(void);

/* fb-con-cprintf.c */
extern void vcprintf(const char *, va_list);
extern void cprintf(const char *, ...);
extern void warn(const char *, ...);

/* hpet.c */
extern bool hpet_init(uintptr_t);

/* mem-heap.c */
extern void *mem_heap_alloc(size_t);
extern void mem_heap_free(void *);
extern void *mem_heap_realloc(void *, size_t);

/* mem-map.c */
extern void *mem_map_reserve_page(uint64_t);
extern void *mem_map_reserve_page_anywhere(void);
extern INIT_TEXT void mem_map_free_bs(void);
extern INIT_TEXT void mem_map_get_cacheability(uint64_t, uint64_t *,
    bool *, bool *);

/* paging.c */
extern INIT_TEXT uint64_t *paging_init(uintptr_t);
extern void paging_extend(uintptr_t);

/* panic.c */
extern NORETURN void vpanic_with_far_caller(uint16_t, void *, const char *,
    va_list);
extern NORETURN void panic_with_far_caller(uint16_t, void *,
    const char *, ...);
extern NORETURN void vpanic_with_caller(void *, const char *, va_list);
extern NORETURN void panic_with_caller(void *, const char *, ...);
extern NORETURN void panic(const char *, ...);
extern NORETURN void assert_fail(const char *);

/* stage1.c */
extern INIT_TEXT void stage1(const void **, uintptr_t *);

/* stage2.c */
extern INIT_TEXT void stage2(uintptr_t);

/* stage3.c */
extern INIT_TEXT void stage3(const void *);

/* lm86-rm86.S */
typedef struct __attribute__((packed)) {
	uint32_t edi, esi, ebp, reserved1, ebx, edx, ecx, eax;
	uint16_t es, ds, fs, gs;
	uint32_t eflags, esp;
	uint16_t ss, ip, cs;
} rm86_regs_t;
typedef void lm86_isr_t(uint16_t caller_cs, void *caller,
    uint64_t error_code, uint8_t int_num);

extern INIT_TEXT void lm86_rm86_init(void *reserved_base_mem,
    uint64_t *pml4);
extern void lm86_set_isr(uint8_t int_num, lm86_isr_t *isr);
extern rm86_regs_t *rm86_regs(void);
extern void rm86(void);

#endif	/* !__ASSEMBLER__ */

#endif	/* H_TRUCKLOAD */
