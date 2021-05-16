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
	uint32_t value ALIGN_APIC;
	uint32_t : 0 ALIGN_APIC;
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
#define     LVT_DM_FIXED	0x00000000U
#define     LVT_DM_SMI		0x00000200U
#define     LVT_DM_NMI		0x00000400U
#define     LVT_DM_INIT		0x00000500U
#define     LVT_DM_EXTINT	0x00000700U
#define LVT_PENDING		0x00001000U	/* delivery status: whether
						   interrupt pending */
#define LVT_PIN_POLARITY	0x00002000U	/* input pin polarity */
#define     LVT_ACTIVE_HIGH	0U
#define     LVT_ACTIVE_LOW	LVT_PIN_POLARITY
#define LVT_REMOTE_IRR		0x00004000U	/* remote IRR */
#define LVT_TRIGGER_MODE	0x00008000U	/* trigger mode */
#define     LVT_EDGE_TRIG	0U
#define     LVT_LEVEL_TRIG	LVT_TRIGGER_MODE
#define LVT_MASKED		0x00010000U	/* whether interrupt masked */

/* I/O APIC indirectly-addressed registers. */
#define IOAPICID		0x00		/* identification */
#define IOAPICVER		0x01		/* version */
#define IOAPICARB		0x02		/* arbitration */
#define IOREDTBLLO(idx)		(0x10 + 2 * (idx)) /* I/O redirection table */
#define IOREDTBLHI(idx)		(0x10 + 2 * (idx) + 1)

/* Field values for I/O APIC redirection table entries. */
#define IORLO_DM		LVT_DM		/* delivery modes */
#define     IORLO_DM_FIXED	LVT_DM_FIXED
#define     IORLO_DM_LOW_PRIO	0x00000100U
#define     IORLO_DM_SMI	LVT_DM_SMI
#define     IORLO_DM_NMI	LVT_DM_NMI
#define     IORLO_DM_INIT	LVT_DM_INIT
#define     IORLO_DM_EXTINT	LVT_DM_EXTINT
#define IORLO_DS		0x00000800U	/* destination mode */
#define     IORLO_DS_PHY	0U
#define     IORLO_DS_LOG	IORLO_DS
#define IORLO_PENDING		LVT_PENDING	/* delivery status: whether
						   interrupt pending */
#define IORLO_PIN_POLARITY	LVT_PIN_POLARITY /* input pin polarity */
#define     IORLO_ACTIVE_HIGH	LVT_ACTIVE_HIGH
#define     IORLO_ACTIVE_LOW	LVT_ACTIVE_LOW
#define IORLO_REMOTE_IRR	LVT_REMOTE_IRR	/* remote IRR */
#define IORLO_TRIGGER_MODE	LVT_TRIGGER_MODE
#define     IORLO_EDGE_TRIG	LVT_EDGE_TRIG
#define     IORLO_LEVEL_TRIG	LVT_LEVEL_TRIG
#define IORLO_MASKED		LVT_MASKED	/* whether interrupt masked */
#define IORHI_DEST		0xff000000U
#define IORHI_DEST_SPEC(dest)	((uint32_t)(uint8_t)(dest) << 24)

/* I/O APIC memory-mapped registers. */
typedef volatile struct __attribute__((packed)) {
	uint32_t IOREGSEL ALIGN_APIC;	/* 0x0000 */
	uint32_t IOREGWIN ALIGN_APIC;	/* 0x0010 */
} ioapic_t;

/* Information on an I/O APIC. */
typedef struct {
	uint8_t id;
	uint8_t max_redir_idx;
	uint32_t irq_base;
	ioapic_t *ioapic;
} ioapic_info_t;

static INIT_DATA bool i8259_compat_p = false;
static lapic_t *lapic = NULL;
static INIT_DATA unsigned max_ioapics = 0;
static unsigned n_ioapics = 0;
static ioapic_info_t *ioapic_info = NULL;
static uint32_t i8259_to_apic_irq_map[16] =
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };

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

/*
 * Given a global IRQ number, find the I/O APIC responsible for this IRQ,
 * & the IRQ's index into the I/O redirection table.
 */
static void ioapic_find_for_irq(uint32_t irq, ioapic_info_t **info,
    uint8_t *redir_idx)
{
	unsigned i = max_ioapics;
	uint32_t offset;
	while (i > 0) {
		uint32_t irq_base;
		--i;
		irq_base = ioapic_info[i].irq_base;
		if (irq_base <= irq) {
			offset = irq - irq_base;
			if (offset > ioapic_info[i].max_redir_idx)
				break;
			*info = &ioapic_info[i];
			*redir_idx = (uint8_t)offset;
			return;
		}
	}
	panic("no IOAPIC responsible for IRQ %#" PRIx32 "!", irq);
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
	ioapic_t *ioapic = (ioapic_t *)addr;
	uint32_t ioapic_ver, ioapic_arb;
	uint8_t max_redir_idx, idx;

	assert(n_ioapics < max_ioapics);

	/* Get information on this I/O APIC. */
	cprintf("starting IOAPIC  id.: %#" PRIx8 "  ", id);
	paging_extend(addr + sizeof(ioapic_t));
	ioapic->IOREGSEL = IOAPICVER;
	ioapic_ver = ioapic->IOREGWIN;
	ioapic->IOREGSEL = IOAPICARB;
	ioapic_arb = ioapic->IOREGWIN;
	cprintf("ver.: %#" PRIx32 "  arb.: %#" PRIx32 "\n",
	    ioapic_ver, ioapic_arb);
	max_redir_idx = (uint8_t)(ioapic_ver >> 16);
	if (max_redir_idx > 0x77) {
		warn("IOAPIC: cannot handle %#x > 0x78 redir. ents.",
		    (unsigned)max_redir_idx + 1);
		max_redir_idx = 0x77;
	}

	/* Disable all IRQs first, on this I/O APIC. */
	for (idx = 0; idx <= max_redir_idx; ++idx) {
		ioapic->IOREGSEL = IOREDTBLLO(idx);
		ioapic->IOREGWIN |= IORLO_MASKED;
	}

	/*
	 * Register this I/O APIC in our list of APICs.  Make sure the list
	 * is sorted by IRQ base.
	 */
	i = n_ioapics;
	while (i > 0 && ioapic_info[i - 1].irq_base > irq_base) {
		ioapic_info[i] = ioapic_info[i - 1];
		--i;
	}
	if (i > 0 && ioapic_info[i - 1].irq_base == irq_base)
		panic("two IOAPICs with same IRQ base!");
	ioapic_info[i] =
	    (ioapic_info_t){ id, max_redir_idx, irq_base, ioapic };
	++n_ioapics;
}

INIT_TEXT void apic_add_ioapic_nmi(uint32_t irq, bool active_low_p,
    bool lvl_trig_p)
{
	ioapic_info_t *info;
	ioapic_t *ioapic;
	uint8_t idx;
	uint32_t value;
	ioapic_find_for_irq(irq, &info, &idx);
	cprintf("setting up NMI on IOAPIC %#" PRIx8 " idx. %#" PRIx8 "\n",
	    info->id, idx);
	ioapic = info->ioapic;
	ioapic->IOREGSEL = IOREDTBLHI(idx);
	ioapic->IOREGWIN = (ioapic->IOREGWIN & ~IORHI_DEST) |
			   IORHI_DEST_SPEC(lapic->ID >> 24);
	ioapic->IOREGSEL = IOREDTBLLO(idx);
	value = ioapic->IOREGWIN;
	value &= ~(IORLO_DM | IORLO_DS | IORLO_PIN_POLARITY |
		   IORLO_TRIGGER_MODE | IORLO_MASKED);
	value |= IORLO_DM_NMI | IORLO_DS_PHY |
		 (active_low_p ? IORLO_ACTIVE_LOW : IORLO_ACTIVE_HIGH) |
		 (lvl_trig_p ? IORLO_LEVEL_TRIG : IORLO_EDGE_TRIG);
	ioapic->IOREGWIN = value;
}

INIT_TEXT void apic_add_irq_override(uint8_t bus, uint8_t src_irq,
    uint32_t apic_irq, bool active_low_p, bool lvl_trig_p)
{
	if (!i8259_compat_p) {
		warn("remap IRQs without i8259 compatibility?");
		return;
	}
	if (bus != 0) {
		warn("unknown bus type %#" PRIx8, bus);
		return;
	}
	if (src_irq > 15)
		panic("no idea how to remap ISA IRQ %" PRIu8 " > 15", src_irq);
	i8259_to_apic_irq_map[src_irq] = apic_irq;
}
