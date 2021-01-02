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

#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <uchar.h>

#define INIT_TEXT	__attribute__((section(".text.i")))
#define INIT_DATA	__attribute__((section(".data.i")))
#define NORETURN	__attribute__((noreturn))

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

/* acpi.c */
extern INIT_TEXT void acpi_init(const void *);

/* fb-con.c */
extern INIT_TEXT void fb_con_init(void);
extern INIT_TEXT bool fb_con_up_p(void);
extern INIT_TEXT void fb_con_exit(void);
extern void putwch(char16_t);
extern void cputws(const char16_t *);
extern void cnputs(const char *, size_t);
extern void cputs(const char *);
extern enum COLORS textcolor(enum COLORS);

/* fb-con-cprintf.c */
extern void vcprintf(const char *, va_list);
extern void cprintf(const char *, ...);

/* mem-heap.c */
extern void *mem_heap_alloc(size_t);
extern void mem_heap_free(void *);
extern void *mem_heap_realloc(void *, size_t);

/* panic.c */
extern NORETURN void panic(const char *, ...);

/* stage1.c */
extern INIT_TEXT void stage1(const void **);

/* stage2.c */
extern INIT_TEXT void stage2(const void *);

/* x64.S */
typedef struct __attribute__((packed)) {
	uint32_t edi, esi, ebp, reserved1, ebx, edx, ecx, eax;
	uint16_t es, ds, fs, gs;
	uint32_t eflags, esp;
	uint16_t ss, ip, cs;
} rm86_regs_t;

extern INIT_TEXT void lm86_rm86_init(uint16_t);
extern rm86_regs_t *rm86_regs(void);
extern void rm86(void);

#endif
