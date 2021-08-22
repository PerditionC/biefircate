/*
 * Copyright (c) 2021 TK Chia
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of the developer(s) nor the names of its
 *     contributors may be used to endorse or promote products derived from
 *     this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "acpi.h"
#include "common.h"
#include "stage2/stage2.h"

/* Legacy 8259 programmable interrupt controller (PIC) I/O port numbers. */
#define PIC1_CMD	0x0020
#define PIC1_DATA	0x0021
#define PIC2_CMD	0x00a0
#define PIC2_DATA	0x00a1

/* ICW1 bit fields for the PICs. */
#define ICW1_IC4	0x01		/* ICW4 needed */
#define ICW1_SNGL	0x02		/* single (vs. cascade) mode */
#define ICW1_ADI	0x04		/* interrupt vector size */
#define ICW1_LTIM	0x08		/* level (vs. edge) triggered */
#define ICW1_INIT	0x10		/* ICW1 is being issued */

/* ICW2: our default base vector addresses. */
#define IRQ0		0x08
#define IRQ8		0x70

/* ICW4 bit fields for the PICs. */
#define ICW4_X86	0x01		/* 8086 (vs. 8080/8085) mode */
#define ICW4_EOI	0x02		/* auto EOI */

/* OCW2 bit fields for the PICs. */
#define OCW2_EOI	0x20		/* non-specific EOI */

/* 8253/8254 programmable interval timer (PIT) I/O port numbers. */
#define PIT_DATA0	0x40
#define PIT_DATA1	0x41
#define PIT_DATA2	0x42
#define PIT_CMD		0x43

/* PIT command bit fields. */
#define PITC_SEL0	0x00		/* select channel 0 */
#define PITC_SEL1	0x40		/* select channel 1 */
#define PITC_SEL2	0x80		/* select channel 2 */
#define PITC_LATCH	0x00		/* counter latch (?) */
#define PITC_LO		0x10		/* low byte only */
#define PITC_HI		0x20		/* high byte only */
#define PITC_LOHI	0x30		/* low byte then high byte */
#define PITC_MODE3	0x06		/* mode 3 (square wave) */
#define PITC_BCD	0x01		/* BCD (vs. binary) mode */

#define ALIGN_APIC	__attribute__((aligned(0x10)))

/* I/O APIC memory-mapped registers. */
typedef volatile struct __attribute__((packed)) {
	uint32_t IOREGSEL ALIGN_APIC;	/* 0x0000 --- I/O register select */
	uint32_t IOREGWIN ALIGN_APIC;	/* 0x0010 --- I/O window */
} ioapic_t;

/* I/O APIC indirectly-addressed registers --- ioapic_t::IOREGSEL values. */
#define IOAPICID	0x00		/* identification */
#define IOAPICVER	0x01		/* version */
#define IOAPICARB	0x02		/* arbitration */
#define IOREDTBLLO(idx)	(0x10 + 2 * (idx))  /* I/O redirection table */
#define IOREDTBLHI(idx)	(0x10 + 2 * (idx) + 1)

/* Field values for I/O APIC redirection table entries. */
#define IOAPIC_RTLO_MASKED 0x00010000U	/* whether interrupt is masked */

/*
 * Map an entire ACPI system description table from physical memory into
 * virtual memory.
 */
static acpi_table_union_t *acpi_map_tab(ptr64_t tab64)
{
	size_t sz;
	acpi_table_union_t *tab;
	tab = mem_va_map(tab64, sizeof(acpi_table_union_t), 0);
	sz = tab->header.length;
	if (sz > sizeof(acpi_table_union_t)) {
		mem_va_unmap(tab, sizeof(acpi_table_union_t));
		tab = mem_va_map(tab64, sz, 0);
	}
	return tab;
}

/*
 * Unmap an ACPI system description table previously mapped with
 * acpi_map_tab(.).
 */
static void acpi_unmap_tab(acpi_table_union_t *tab)
{
	size_t sz = tab->header.length;
	if (sz < sizeof(acpi_table_union_t))
		sz = sizeof(acpi_table_union_t);
	mem_va_unmap(tab, sz);
}

static void acpi_process_madt(acpi_madt_t *madt)
{
	char *madt_end, *ic;
	uint32_t ioapic_phy;
	ioapic_t *ioapic;
	unsigned io_intr;
	/*
	 * Go through the interrupt controller structures.  Mask I/O APIC
	 * interrupts.
	 *
	 * FIXME: I am not sure of the correct protocol to switch from APIC
	 * mode to legacy 8259 mode.  Tests seem to suggest that I can just
	 * disable I/O APIC interrupts --- while _not_ disabling the local
	 * APIC(s) --- & then program the 8259 PICs.  But is there a more
	 * rigorous way?  -- 20210821
	 */
	madt_end = (char *)madt + madt->header.length;
	ic = madt->ics;
	while (ic != madt_end) {
		acpi_madt_ic_union_t *u = (acpi_madt_ic_union_t *)ic;
		switch (u->header.type) {
		    case MADT_IC_IOAPIC:
			ioapic_phy = u->ioapic.ioapic_phy_addr;
			ioapic = mem_va_map(ioapic_phy, sizeof(ioapic_t),
			    PTE_CD);
			for (io_intr = 0; io_intr < 24; ++io_intr) {
				ioapic->IOREGSEL = IOREDTBLLO(io_intr);
				ioapic->IOREGWIN |= IOAPIC_RTLO_MASKED;
			}
			mem_va_unmap(ioapic, sizeof(ioapic_t));
			break;
		    default:
			;
		}
		ic += u->header.length;
	}
}

static void acpi_process_xsdt(acpi_xsdt_t *xsdt)
{
	static const char madt_sig[4] = "APIC";
	size_t xsdt_sz, num_tabs, i;
	xsdt_sz = xsdt->header.length;
	num_tabs = (xsdt_sz - sizeof(acpi_header_t)) / sizeof(uint64_t);
	for (i = 0; i < num_tabs; ++i) {
		acpi_table_union_t *tab = acpi_map_tab(xsdt->tables[i]);
		if (memcmp(tab->header.signature, madt_sig, 4) == 0)
			acpi_process_madt(&tab->madt);
		acpi_unmap_tab(tab);
	}
}

static void acpi_process_rsdp(acpi_xsdp_t *rsdp)
{
	const char expect_rsdp_sig[8] = "RSD PTR ";
	acpi_table_union_t *xsdt;
	if (memcmp(rsdp->signature, expect_rsdp_sig, 8) != 0)
		hlt();
	xsdt = acpi_map_tab(rsdp->xsdt);
	acpi_process_xsdt(&xsdt->xsdt);
	acpi_unmap_tab(xsdt);
}

void irq_init(bparm_t *bparms)
{
	/* Find the ACPI RSDP from the boot parameters. */
	bdat_rsdp_t *bd_rsdp;
	acpi_xsdp_t *rsdp;
	uint32_t rsdp_sz;
	bparm_t *bp = bparms;
	while (bp->type != BP_RSDP)
		bp = bp->next;
	bd_rsdp = &bp->u->rsdp;
	rsdp_sz = bd_rsdp->rsdp_sz;
	rsdp = mem_va_map(bd_rsdp->rsdp_phy_addr, rsdp_sz, 0);
	/* Process the RSDP to disable APIC interrupts. */
	acpi_process_rsdp(rsdp);
	mem_va_unmap(rsdp, rsdp_sz);
	/*
	 * Bring up the legacy 8259 interrupt controllers.
	 *
	 * FIXME: also need to set interrupt edge/level sensitivity via ports
	 * 0x4d0 & 0x4d1?  TianoCore's EDK II code does do this.  -- 20210821
	 */
	outp_w(PIC1_CMD, ICW1_INIT | ICW1_IC4);	/* ICW1 */
	outp_w(PIC2_CMD, ICW1_INIT | ICW1_IC4);
	outp_w(PIC1_DATA, IRQ0);		/* ICW2 */
	outp_w(PIC2_DATA, IRQ8);
	outp_w(PIC1_DATA, 1 << 2);		/* ICW3 */
	outp_w(PIC2_DATA, 1 << 1);
	outp_w(PIC1_DATA, ICW4_X86);		/* ICW4 */
	outp_w(PIC2_DATA, ICW4_X86);
	/* Set the IRQ masks. */
	outp_w(PIC1_DATA, ~(1 << 0 | 1 << 1));	/* OCW1 */
	outp_w(PIC2_DATA, ~0);
	/* Send EOIs for good measure. */
	outp_w(PIC1_CMD, OCW2_EOI);		/* OCW2 */
	outp_w(PIC2_CMD, OCW2_EOI);
	/* Program the 8253/8254 PIT for 18.2 Hz operation on IRQ 0. */
	outp_w(PIT_CMD, PITC_SEL0 | PITC_LOHI | PITC_MODE3);
	outp_w(PIT_DATA0, 0x00);
	outp_w(PIT_DATA0, 0x00);
}
