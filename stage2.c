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

#include "truckload.h"

INIT_TEXT void stage2(uintptr_t mapped_mem_end)
{
	void *reserved_base_mem;
	uint64_t *pml4 = paging_init(mapped_mem_end);
	mem_map_free_bs();
	reserved_base_mem = mem_map_reserve_page(0xf0000ULL);
	cprintf("installing LM \u2194 RM trampolines @%p; starting new "
		"LM env.\n", reserved_base_mem);
	disable();
	lm86_rm86_init(reserved_base_mem, pml4);
}
