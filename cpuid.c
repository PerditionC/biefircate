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
#include "truckload.h"

uint32_t max_std_leaf = 0;

INIT_TEXT void cpuid_init(void)
{
	cpuid_t id = cpuid(0);
	max_std_leaf = id.a;
	cprintf("CPUID  max. std. leaf: %#" PRIx32 "  "
		"vendor: %4.4s%4.4s%4.4s\n",
	    id.a, id.b_bytes, id.d_bytes, id.c_bytes);
}

INIT_TEXT bool cpuid_has_std_leaf_p(uint32_t leaf)
{
	return leaf <= max_std_leaf;
}
