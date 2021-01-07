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

#ifndef H_EFI_STUFF
#define H_EFI_STUFF

#include <efi.h>
#include "truckload.h"

/* mem-heap.c */
extern INIT_TEXT void mem_heap_init(void);

/* mem-map.c */
extern INIT_TEXT void mem_map_init(UINTN *);
extern INIT_TEXT void stage1_done(UINTN);

/* panic.c */
extern NORETURN INIT_TEXT void panic_efi(const char *, EFI_STATUS);

#endif
