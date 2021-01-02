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
#include "efi-stuff.h"
#include "truckload.h"

NORETURN void panic(const char *fmt, ...)
{
	extern EFI_STATUS _start(EFI_HANDLE, EFI_SYSTEM_TABLE *);
	char *ra = __builtin_return_address(0);
	va_list ap;
	textcolor(WHITE);
	cputs("panic: ");
	va_start(ap, fmt);
	vcprintf(fmt, ap);
	va_end(ap);
	cprintf("\n[caller @%p (= @%p+%#tx)]",
	    ra, _start, (char *)ra - (char *)_start);
	freeze();
}

NORETURN INIT_TEXT void panic_efi(const char *msg, EFI_STATUS status)
{
	panic("%s: EFI_STATUS %#" PRIx64, (uint64_t)status);
}
