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

#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include "efi-stuff.h"
#include "truckload.h"

NORETURN void vpanic_with_far_caller(uint16_t caller_cs, void *caller,
    const char *fmt, va_list ap)
{
	bool our_cs_p = (caller_cs == get_cs());
	extern EFI_STATUS _start(EFI_HANDLE, EFI_SYSTEM_TABLE *);
	textcolor(WHITE);
	cputs("panic: ");
	vcprintf(fmt, ap);
	va_end(ap);
	cprintf("  [\u2190 @%#" PRIx16 ":%p", caller_cs, caller);
	if (our_cs_p)
		cprintf(" = _start+%#tx", (char *)caller - (char *)_start);
	putwch(u']');
	freeze();
}

NORETURN void panic_with_far_caller(uint16_t caller_cs, void *caller,
    const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vpanic_with_far_caller(caller_cs, caller, fmt, ap);
}

NORETURN void vpanic_with_caller(void *caller, const char *fmt, va_list ap)
{
	vpanic_with_far_caller(get_cs(), caller, fmt, ap);
}

NORETURN void panic_with_caller(void *caller, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vpanic_with_caller(caller, fmt, ap);
}

NORETURN void panic(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vpanic_with_caller(__builtin_return_address(0), fmt, ap);
}

NORETURN INIT_TEXT void panic_efi(const char *msg, EFI_STATUS status)
{
	panic_with_caller(__builtin_return_address(0),
	    "%s: EFI_STATUS %#" PRIx64, msg, (uint64_t)status);
}
