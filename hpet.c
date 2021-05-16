/*
 * Copyright (c) 2021 TK Chia
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
#include <stdbool.h>
#include "truckload.h"

typedef struct __attribute__((packed)) {
	uint8_t REV;
	uint8_t NUM : 5, BITS64 : 1, : 1, LEG_RT : 1;
	uint16_t VENDOR;
	uint32_t PERIOD;
} hpet_cap_id_t;

typedef volatile struct __attribute__((packed)) {
	uint64_t CFG;
	uint64_t CMP;
	uint64_t ROUTE;
	uint64_t : 64;
} hpet_timer_t;

/*
 * HPET memory-mapped registers.  I capitalize the field names to highlight
 * that these do not refer to ordinary memory.
 */
typedef volatile struct __attribute__((packed)) {
	volatile hpet_cap_id_t ID;	/* 0x0000 */
	uint64_t : 64;
	uint64_t CFG, : 64;		/* 0x0010 */
	uint64_t STATUS, : 64;		/* 0x0020 */
	uint64_t : 64, : 64;		/* 0x0030 */
	uint64_t : 64, : 64, : 64, : 64;/* 0x0040 */
	uint64_t : 64, : 64, : 64, : 64;/* 0x0060 */
	uint64_t : 64, : 64, : 64, : 64;/* 0x0080 */
	uint64_t : 64, : 64, : 64, : 64;/* 0x00a0 */
	uint64_t : 64, : 64, : 64, : 64;/* 0x00c0 */
	uint64_t : 64, : 64;		/* 0x00e0 */
	uint64_t COUNTER, : 64;		/* 0x00f0 */
	hpet_timer_t T[];		/* 0x0100 */
} hpet_t;

static hpet_t *hpet = NULL;
static uint32_t hpet_period = 0;

INIT_TEXT bool hpet_init(uintptr_t hpet_addr)
{
	hpet_t *h = (hpet_t *)hpet_addr;
	hpet_cap_id_t id = h->ID;
	cprintf("HPET @%p:\n"
		"  rev.: %" PRIu8 "  last timer: %u  period: %" PRIu32 " fs  "
		  "{ %s%s}\n",
	    h, id.REV, (unsigned)id.NUM, id.PERIOD,
	    id.BITS64 ? "64-bit " : "",
	    id.LEG_RT ? "leg-rt " : "");
	if (!id.REV || !id.PERIOD)
		return false;
	hpet = h;
	return true;
}
