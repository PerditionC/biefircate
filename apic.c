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

#define ALIGN_APIC	__attribute__((aligned(0x10)))

typedef volatile struct __attribute__((packed))
{
	uint32_t value ALIGN_APIC, : 0;
} wrapped_apic_reg32_t;

/*
 * Local APIC memory-mapped registers.  I capitalize the field names to
 * highlight that these do not refer to ordinary memory.
 */
typedef volatile struct __attribute__((packed)) {
	uint32_t : 32 ALIGN_APIC;	/* 0x0000 */
	uint32_t : 32 ALIGN_APIC;
	uint32_t ID ALIGN_APIC;
	uint32_t VERSION ALIGN_APIC;
	uint32_t : 32 ALIGN_APIC;	/* 0x0040 */
	uint32_t : 32 ALIGN_APIC;
	uint32_t : 32 ALIGN_APIC;
	uint32_t : 32 ALIGN_APIC;
	uint32_t TPR ALIGN_APIC;	/* 0x0080 */
	uint32_t APR ALIGN_APIC;
	uint32_t PPR ALIGN_APIC;
	uint32_t EOI ALIGN_APIC;
	uint32_t RRD ALIGN_APIC;	/* 0x00c0 */
	uint32_t LDR ALIGN_APIC;
	uint32_t DFR ALIGN_APIC;
	uint32_t SVR ALIGN_APIC;
	wrapped_apic_reg32_t ISR[8];	/* 0x0100 */
	wrapped_apic_reg32_t TMR[8];	/* 0x0180 */
	wrapped_apic_reg32_t IRR[8];	/* 0x0200 */
	uint32_t ESR ALIGN_APIC;	/* 0x0280 */
	uint32_t : 32 ALIGN_APIC;
	uint32_t : 32 ALIGN_APIC;
	uint32_t : 32 ALIGN_APIC;
	uint32_t : 32 ALIGN_APIC;	/* 0x02c0 */
	uint32_t : 32 ALIGN_APIC;
	uint32_t : 32 ALIGN_APIC;
	uint32_t LVT_CMCI ALIGN_APIC;
	wrapped_apic_reg32_t ICR[2];	/* 0x0300 */
	uint32_t LVT_TMR ALIGN_APIC;
	uint32_t LVT_THRM ALIGN_APIC;
	uint32_t LVT_PMC ALIGN_APIC;	/* 0x0340 */
	wrapped_apic_reg32_t LVT_LINT[2];
	uint32_t LVT_ERR ALIGN_APIC;
	uint32_t TMR_IC ALIGN_APIC;	/* 0x0380 */
	uint32_t TMR_CC ALIGN_APIC;
	uint32_t : 32 ALIGN_APIC;
	uint32_t : 32 ALIGN_APIC;
	uint32_t : 32 ALIGN_APIC;	/* 0x03c0 */
	uint32_t : 32 ALIGN_APIC;
	uint32_t TMR_DC ALIGN_APIC;
	uint32_t : 32 ALIGN_APIC;
} lapic_t;

/* Field values for the spurious interrupt vector register (->SVR). */
#define SVR_APIC_ENABLE		0x00000100U
#define SVR_NO_FOCUS_CHECKING	0x00000200U
#define SVR_NO_EOI_BROADCAST	0x00001000U

/* Field values for local vector table registers. */
#define LVT_VECTOR		0x000000ffU	/* bit mask for vector */
#define LVT_DM			0x00000700U	/* delivery modes */
#define    LVT_DM_FIXED		0x00000000U
#define    LVT_DM_SMI		0x00000200U
#define    LVT_DM_NMI		0x00000400U
#define    LVT_DM_INIT		0x00000500U
#define    LVT_DM_EXTINT	0x00000700U
#define LVT_PENDING		0x00001000U	/* delivery status: whether
						   interrupt pending */
#define LVT_PIN_POLARITY	0x00002000U	/* input pin polarity */
#define    LVT_ACTIVE_HIGH	0U
#define    LVT_ACTIVE_LOW	LVT_PIN_POLARITY
#define LVT_REMOTE_IRR		0x00004000U	/* remote IRR */
#define LVT_TRIGGER_MODE	0x00008000U	/* trigger mode */
#define    LVT_EDGE_TRIG	0U
#define    LVT_LEVEL_TRIG	LVT_TRIGGER_MODE
#define LVT_MASKED		0x00010000U	/* whether interrupt masked */

/* I/O APIC memory-mapped registers. */
typedef volatile struct __attribute__((packed)) {
	uint32_t IOREGSEL ALIGN_APIC;	/* 0x0000 */
	uint32_t IOREGWIN ALIGN_APIC;	/* 0x0010 */
} ioapic_t;

/* Information on an I/O APIC. */
typedef struct {
	uint8_t id;
	uint32_t irq_base;
	ioapic_t *mem;
} ioapic_info_t;

static INIT_DATA bool i8259_compat_p = false;
static lapic_t *lapic = NULL;
static INIT_DATA unsigned max_ioapics = 0;
static unsigned n_ioapics = 0;
static ioapic_info_t *ioapic_info = NULL;

static inline void lapic_enable_int_as_nmi(volatile uint32_t *lapic_reg,
    bool active_low_p, bool lvl_trig_p)
{
	uint32_t value = *lapic_reg;
	if (lvl_trig_p)
		warn("requesting to make NMI level-triggered?");
	value &= ~(LVT_DM | LVT_PIN_POLARITY | LVT_TRIGGER_MODE |
		   LVT_MASKED);
	value |= LVT_DM_NMI |
		 (active_low_p ? LVT_ACTIVE_LOW : LVT_ACTIVE_HIGH) |
		 (lvl_trig_p ? LVT_LEVEL_TRIG : LVT_EDGE_TRIG);
	*lapic_reg = value;
}

static INIT_TEXT void lapic_init(int_fast16_t lapic_nmi_lint,
    bool lapic_nmi_active_low_p, bool lapic_nmi_lvl_trig_p)
{
	cprintf("starting LAPIC  id.: %#" PRIx32 "  ver.: %#" PRIx32 "\n",
	    lapic->ID, lapic->VERSION);

	/* Probably good to disable everything first... */
	lapic->SVR &= ~SVR_APIC_ENABLE;
	lapic->LVT_CMCI |= LVT_MASKED;
	lapic->LVT_TMR |= LVT_MASKED;
	lapic->LVT_THRM |= LVT_MASKED;
	lapic->LVT_PMC |= LVT_MASKED;
	lapic->LVT_LINT[0].value |= LVT_MASKED;
	lapic->LVT_LINT[1].value |= LVT_MASKED;
	lapic->LVT_ERR |= LVT_MASKED;

	/* Set up the LAPIC's NMI. */
	if (lapic_nmi_lint < 0)
		warn("no LAPIC NMI LINT#?");
	else if (lapic_nmi_lint > 1)
		warn("LAPIC NMI LINT# %#" PRIxFAST16 " looks bogus "
		     "(not 0 or 1)", lapic_nmi_lint);
	else {
		cprintf("  LAPIC NMI LINT# %#" PRIxFAST16, lapic_nmi_lint);
		lapic_enable_int_as_nmi(&lapic->LVT_LINT[lapic_nmi_lint].value,
		    lapic_nmi_active_low_p, lapic_nmi_lvl_trig_p);
	}
	putwch(u'\n');
}

INIT_TEXT void apic_init(uintptr_t lapic_addr, int_fast16_t lapic_nmi_lint,
    bool lapic_nmi_active_low_p, bool lapic_nmi_lvl_trig_p, bool i8259_p,
    unsigned max_ioics)
{
	if (!max_ioics)
		panic("no IOAPICs?");
	max_ioapics = max_ioics;
	lapic = (lapic_t *)lapic_addr;
	paging_extend(lapic_addr + sizeof(lapic_t));

	/*
	 * Initialize & turn off legacy i8259 Programmable Interrupt
	 * Controller support --- but only if there is legacy i8259 support.
	 *
	 * Assume that interrupts are currently disabled.
	 */
	i8259_compat_p = i8259_p;
	if (i8259_p) {
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

	/* Initialize the local APIC for the current core. */
	lapic_init(lapic_nmi_lint,
	    lapic_nmi_active_low_p, lapic_nmi_lvl_trig_p);

	/* Reserve space for information on I/O APICs. */
	ioapic_info =
	    mem_heap_alloc((size_t)max_ioics * sizeof(ioapic_info_t));
}

INIT_TEXT void apic_add_ioapic(uint8_t id, uintptr_t addr, uint32_t irq_base)
{
	unsigned i;
	assert(n_ioapics < max_ioapics);
	i = n_ioapics;
	while (i > 0 && ioapic_info[i - 1].irq_base > irq_base) {
		ioapic_info[i] = ioapic_info[i - 1];
		--i;
	}
	if (i > 0 && ioapic_info[i - 1].irq_base == irq_base)
		panic("two IOAPICs with same IRQ base!");
	ioapic_info[i] =
	    (ioapic_info_t){ id, irq_base, (ioapic_t *)addr };
	++n_ioapics;
	paging_extend(addr + sizeof(ioapic_t));
}
