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

/* acpi.c */
extern INIT_TEXT void acpi_init(const void *);

/* apic.c */
extern INIT_TEXT void apic_init(uintptr_t, bool, unsigned);

/* fb-con.c */
extern INIT_TEXT void fb_con_init(void);
extern INIT_TEXT uint64_t fb_con_mem_end(void);
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

/* mem-heap.c */
extern void *mem_heap_alloc(size_t);
extern void mem_heap_free(void *);
extern void *mem_heap_realloc(void *, size_t);

/* mem-map.c */
extern INIT_TEXT void *mem_map_reserve_page(uint64_t);
extern INIT_TEXT void *mem_map_reserve_page_anywhere(void);
extern INIT_TEXT uint64_t mem_map_all_end(void);
extern INIT_TEXT void mem_map_free_bs(void);
extern INIT_TEXT void mem_map_get_cacheability(uint64_t, uint64_t *,
    bool *, bool *);

/* panic.c */
extern NORETURN void vpanic_with_far_caller(uint16_t, void *, const char *,
    va_list);
extern NORETURN void panic_with_far_caller(uint16_t, void *,
    const char *, ...);
extern NORETURN void vpanic_with_caller(void *, const char *, va_list);
extern NORETURN void panic_with_caller(void *, const char *, ...);
extern NORETURN void panic(const char *, ...);

/* stage1.c */
extern INIT_TEXT void stage1(const void **);

/* stage2.c */
extern INIT_TEXT void stage2(void);

/* stage3.c */
extern INIT_TEXT void stage3(const void *);

/* x64.S */
typedef struct __attribute__((packed)) {
	uint32_t edi, esi, ebp, reserved1, ebx, edx, ecx, eax;
	uint16_t es, ds, fs, gs;
	uint32_t eflags, esp;
	uint16_t ss, ip, cs;
} rm86_regs_t;

extern INIT_TEXT void lm86_rm86_init(void *reserved_base_mem,
    uint64_t *pml4);
extern rm86_regs_t *rm86_regs(void);
extern void rm86(void);

#endif	/* !__ASSEMBLER__ */

#endif	/* H_TRUCKLOAD */
