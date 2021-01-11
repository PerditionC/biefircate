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
#include <stdbool.h>
#include "truckload.h"

typedef struct {
	uint32_t irq_base;
	volatile uint32_t *mem;
} ioapic_t;

static volatile uint32_t *lapic = NULL;
static unsigned max_ioapics = 0, n_ioapics = 0;
static ioapic_t *ioapic = NULL;

INIT_TEXT void apic_init(uintptr_t lapic_addr, bool i8259_compat_p,
    unsigned max_ioics)
{
	if (!max_ioics)
		panic("no IOAPICs?");
	max_ioapics = max_ioics;
	lapic = (volatile uint32_t *)lapic_addr;
	/*
	 * Initialize & turn off legacy i8259 Programmable Interrupt
	 * Controller support --- but only if there is legacy i8259 support.
	 *
	 * Assume that interrupts are currently disabled.
	 */
	if (i8259_compat_p) {
		cputs("disabling i8259\n");
		outp(0x20, 0x11);	/* ICW1 for PIC 1 */
		outp(0xa0, 0x11);	/* ICW1 for PIC 2 */
		outp(0x21, 0x68);	/* ICW2 for PIC 1 --- set the legacy
					   IRQ 0 vector base to something
					   sane */
		outp(0xa1, 0x70);	/* ICW2 for PIC 2 --- set the legacy
					   IRQ 8 vector base to something
					   sane */
		outp(0x21, 0x04);	/* ICW3 for PIC 1 */
		outp(0xa1, 0x02);	/* ICW3 for PIC 2 */
		outp(0x21, 0x01);	/* ICW4 for PIC 1 */
		outp(0xa1, 0x01);	/* ICW4 for PIC 2 */
		outp(0x21, 0xff);	/* OCW1 for PIC 1 --- disable legacy
					   IRQs 0--7 */
		outp(0xa1, 0xff);	/* OCW1 for PIC 2 --- disable legacy
					   IRQs 8--15 */
	}
	/* FIXME */
}
