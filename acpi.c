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

#include <efi.h>
#include <efilib.h>
#include <stddef.h>
#include <string.h>
#include "efi-stuff.h"
#include "truckload.h"
#include "acpica-stuff.h"

/* ACPICA header files. */
#include "actypes.h"
#include "actbl.h"
#include "acrestyp.h"
#include "acpixf.h"
#include "acexcep.h"
#include "aclocal.h"
#include "acobject.h"
#include "acstruct.h"
#include "acnamesp.h"
#include "acnames.h"
#include "acpiosxf.h"

/* Used by ACPICA (via acpica-osl.c). */
const ACPI_TABLE_RSDP *acpi_rsdp = NULL;

static const ACPI_TABLE_XSDT *acpi_xsdt = NULL;

static NORETURN void panic_acpi(const char *msg, ACPI_STATUS status)
{
	panic_with_caller(__builtin_return_address(0),
	    "%s failed: ACPI_STATUS %#" PRIx32, msg, (uint32_t)status);
}

static INIT_TEXT void dump_acpi_gen_addr(const ACPI_GENERIC_ADDRESS *addr)
{
	cprintf("{ (%" PRIu8 ") @%#" PRIx64 "[%" PRIu8 "..",
	    addr->SpaceId, addr->Address, addr->BitOffset);
	if (addr->BitWidth)
		cprintf("%" PRIu8, addr->BitOffset + addr->BitWidth - 1);
	cputs("] }");
}

static INIT_TEXT void process_fadt(const ACPI_TABLE_FADT *fadt)
{
	if (!fadt)
		panic("no ACPI FADT?");
	if (fadt->Header.Length < ACPI_FADT_V1_SIZE)
		panic("ACPI FADT is too short: only %#" PRIx32 " bytes",
		    fadt->Header.Length);
	cprintf("ACPI FADT @%p:\n"
		"  SCI int.: %#" PRIx16 "  SMI port: %#" PRIx32 "  "
		  "ACPI enable/disable: 0x%02" PRIx8 "/0x%02" PRIx8 "  "
		  "S4BIOS: 0x%02" PRIx8 "\n"
		"  IA-PC boot flags: 0x%04" PRIx16 " { %s%s%s%s%s%s}\n",
	    fadt, fadt->SciInterrupt,
	    fadt->SmiCommand, fadt->AcpiEnable, fadt->AcpiDisable,
	    fadt->S4BiosRequest,
	    fadt->BootFlags,
	    fadt->BootFlags & ACPI_FADT_LEGACY_DEVICES ? "legacy-devs " : "",
	    fadt->BootFlags & ACPI_FADT_8042 ? "i8042 " : "",
	    fadt->BootFlags & ACPI_FADT_NO_VGA ? "\u00ac""VGA " : "",
	    fadt->BootFlags & ACPI_FADT_NO_MSI ? "\u00ac""MSI " : "",
	    fadt->BootFlags & ACPI_FADT_NO_ASPM ? "\u00ac""ASPM " : "",
	    fadt->BootFlags & ACPI_FADT_NO_CMOS_RTC ? "\u00ac""CMOS-RTC "
						    : "");
}

static void parse_inti_flags(UINT16 flags, int_fast16_t bus,
    bool *active_low_p, bool *lvl_trig_p)
{
	bool warned = false;
	if (active_low_p) {
		switch (flags & 0x03) {
		    case 0x00:
			if (bus != 0) {
				if (bus < 0)
					warn("no bus type given, "
					     "assuming ISA");
				else
					warn("unknown bus type %#" PRIxFAST16
					     ", assuming ISA", bus);
				warned = true;
			}
			/* fall through */
		    case 0x03:
			*active_low_p = true;
			break;
		    case 0x01:
			*active_low_p = false;
			break;
		    default:
			panic("cannot get int. polarity from MPS INTI flags "
			      "%#" PRIx16, flags);
		}
	}
	if (lvl_trig_p) {
		switch (flags & 0x0c) {
		    case 0x00:
			if (bus != 0 && !warned) {
				if (bus < 0)
					warn("no bus type given, "
					     "assuming ISA");
				else
					warn("unknown bus type %#" PRIxFAST16
					     ", assuming ISA", bus);
			}
			/* fall through */
		    case 0x04:
			*lvl_trig_p = false;
			break;
		    case 0x0c:
			*lvl_trig_p = true;
			break;
		    default:
			panic("cannot get int. trigger mode from MPS INTI "
			      "flags %#" PRIx16, flags);
		}
	}
}

#define FOR_EACH_MADT_SUBTABLE(stbl, madt, itr) \
	for ((itr) = (madt + 1), \
		(stbl) = (itr); \
	     (itr) != (const char *)(madt) + (madt)->Header.Length; \
	     (stbl) = (itr), \
		(itr) = (const char *)(itr) + (stbl)->Length, \
		(stbl) = (itr))

static INIT_TEXT void process_madt(const ACPI_TABLE_MADT *madt)
{
	const void *itr;
	const ACPI_SUBTABLE_HEADER *stbl;
	uintptr_t lapic_addr;
	bool i8259_compat_p;
	int_fast16_t lapic_nmi_lint = -1;
	bool lapic_nmi_active_low_p = true, lapic_nmi_lvl_trig_p = false;
	unsigned n_ioapics = 0;

	if (!madt)
		panic("no ACPI MADT?");

	/* Dump the MADT to the console, & start to collect information. */
	lapic_addr = madt->Address;
	i8259_compat_p = ((madt->Flags & ACPI_MADT_PCAT_COMPAT) != 0);
	cprintf("ACPI MADT @%p:\n"
		"  LAPIC: @%#" PRIxPTR "  flags: 0x%08" PRIx32 " { %s}\n",
	    madt, lapic_addr, madt->Flags, i8259_compat_p ? "i8259 " : "");
	FOR_EACH_MADT_SUBTABLE(stbl, madt, itr) {
		switch (stbl->Type) {
		    case ACPI_MADT_TYPE_LOCAL_APIC:
			{
				const ACPI_MADT_LOCAL_APIC *ic =
				    (const ACPI_MADT_LOCAL_APIC *)stbl;
				cprintf("  LAPIC { proc. uid.: %#" PRIx8 "  "
						  "APIC id.: %#" PRIx8 "  "
						  "flags: 0x%08" PRIx32
						  " { %s%s} }\n",
				    ic->ProcessorId, ic->Id,
				    ic->LapicFlags,
				    ic->LapicFlags & 1 ? "enabled " : "",
				    ic->LapicFlags & 2 ? "online-capable "
						       : "");
			}
			break;
		    case ACPI_MADT_TYPE_IO_APIC:
			{
				const ACPI_MADT_IO_APIC *ic =
				    (const ACPI_MADT_IO_APIC *)stbl;
				cprintf("  IOAPIC { @%#" PRIx32 "  "
						   "APIC id.: %#" PRIx8 "  "
						   "IRQ base: %#" PRIx32
						   " }\n",
				    ic->Address, ic->Id, ic->GlobalIrqBase);
				++n_ioapics;
			}
			break;
		    case ACPI_MADT_TYPE_INTERRUPT_OVERRIDE:
			{
				const ACPI_MADT_INTERRUPT_OVERRIDE *ic =
				    (const ACPI_MADT_INTERRUPT_OVERRIDE *)stbl;
				UINT8 src_irq = ic->SourceIrq, bus = ic->Bus;
				UINT32 apic_irq = ic->GlobalIrq;
				UINT16 inti_fl = ic->IntiFlags;
				cprintf("  int. override { "
					  "%#" PRIx8 " \u2192 %#" PRIx32 "  "
					  "bus: %#" PRIx8 "  "
					  "INTI fl.: 0x%04" PRIx16 " }\n",
				    src_irq, apic_irq, bus, inti_fl);
			}
			break;
		    case ACPI_MADT_TYPE_NMI_SOURCE:
			{
				const ACPI_MADT_NMI_SOURCE *ic =
				    (const ACPI_MADT_NMI_SOURCE *)stbl;
				cprintf("  NMI source { "
					  "INTI fl.: 0x%04" PRIx16 "  "
					  "global IRQ: %#" PRIx32 " }\n",
				    ic->IntiFlags, ic->GlobalIrq);
			}
			break;
		    case ACPI_MADT_TYPE_LOCAL_APIC_NMI:
			{
				const ACPI_MADT_LOCAL_APIC_NMI *ic =
				    (const ACPI_MADT_LOCAL_APIC_NMI *)stbl;
				cprintf("  LAPIC NMI { "
					  "proc. id.: %#" PRIx8 "  "
					  "INTI fl.: 0x%04" PRIx16 "  "
					  "LINT#: %#" PRIx8 " }\n",
				    ic->ProcessorId, ic->IntiFlags, ic->Lint);
				lapic_nmi_lint = ic->Lint;
				parse_inti_flags(ic->IntiFlags, -1,
				    &lapic_nmi_active_low_p,
				    &lapic_nmi_lvl_trig_p);
			}
			break;
		    case ACPI_MADT_TYPE_LOCAL_APIC_OVERRIDE:
			{
				const ACPI_MADT_LOCAL_APIC_OVERRIDE *ic =
				  (const ACPI_MADT_LOCAL_APIC_OVERRIDE *)stbl;
				cprintf("  LAPIC override { @%#" PRIx64 " }\n",
				    ic->Address);
				lapic_addr = ic->Address;
			}
			break;
		    default:
			cprintf("  type %#" PRIx8 " int. ctrlr. struc. @%p\n",
			    stbl->Type, stbl);
		}
	}

	/*
	 * Start setting up the local APIC & I/O APICs, & also disable all
	 * the I/O APICs' IRQs first.
	 */
	apic_init(lapic_addr, lapic_nmi_lint, lapic_nmi_active_low_p,
	    lapic_nmi_lvl_trig_p, i8259_compat_p, n_ioapics);
	FOR_EACH_MADT_SUBTABLE(stbl, madt, itr) {
		if (stbl->Type == ACPI_MADT_TYPE_IO_APIC) {
			const ACPI_MADT_IO_APIC *ic =
			    (const ACPI_MADT_IO_APIC *)stbl;
			apic_add_ioapic(ic->Id, ic->Address,
					ic->GlobalIrqBase);
		}
	}

	/*
	 * We have disabled all the I/O APIC IRQs.  Enable interrupt
	 * handling on the CPU, then slowly bring up (some of) the IRQs as
	 * mentioned in the MADT.
	 */
	enable();
	FOR_EACH_MADT_SUBTABLE(stbl, madt, itr) {
		switch (stbl->Type) {
		    case ACPI_MADT_TYPE_INTERRUPT_OVERRIDE:
			{
				const ACPI_MADT_INTERRUPT_OVERRIDE *ic =
				    (const ACPI_MADT_INTERRUPT_OVERRIDE *)stbl;
				UINT8 src_irq = ic->SourceIrq, bus = ic->Bus;
				UINT32 apic_irq = ic->GlobalIrq;
				UINT16 inti_fl = ic->IntiFlags;
				bool active_low_p, lvl_trig_p;
				parse_inti_flags(inti_fl, bus,
				    &active_low_p, &lvl_trig_p);
				apic_add_irq_override(bus, src_irq, apic_irq,
				    active_low_p, lvl_trig_p);
			}
			break;
		    case ACPI_MADT_TYPE_NMI_SOURCE:
			{
				const ACPI_MADT_NMI_SOURCE *ic =
				    (const ACPI_MADT_NMI_SOURCE *)stbl;
				bool active_low_p, lvl_trig_p;
				parse_inti_flags(ic->IntiFlags, -1,
				    &active_low_p, &lvl_trig_p);
				apic_add_ioapic_nmi(ic->GlobalIrq,
				    active_low_p, lvl_trig_p);
			}
			break;
		    default:
			;
		}
	}
}

static INIT_TEXT void process_hpet(const ACPI_TABLE_HPET *hpet)
{
	static bool hpet_inited = false;
	if (hpet_inited)
		return;
	if (!hpet)
		panic("no usable ACPI HPET?");
	cprintf("ACPI HPET @%p:\n"
		"  seq.: %" PRIu8 "  id.: %#" PRIx32 "  min. tick: %" PRIu16
		"  flags: %#" PRIx8 "\n"
		"  addr: ",
	    hpet, hpet->Sequence, hpet->Id, hpet->MinimumTick, hpet->Flags);
	dump_acpi_gen_addr(&hpet->Address);
	putwch(u'\n');
}

static INIT_TEXT void process_xsdt(void)
{
	const char xsdt_sig[4] = "XSDT", fadt_sig[4] = "FACP",
		   madt_sig[4] = "APIC", hpet_sig[4] = "HPET";
	const ACPI_TABLE_FADT *fadt = NULL;
	const ACPI_TABLE_MADT *madt = NULL;
	UINT32 n, i;
	if (memcmp(acpi_xsdt->Header.Signature, xsdt_sig, sizeof xsdt_sig)
	    != 0)
		panic("ACPI XSDT has bogus signature");
	n = (acpi_xsdt->Header.Length
	    - offsetof(ACPI_TABLE_XSDT, TableOffsetEntry)) / sizeof(UINT64);
	cputs("  description tables:");
	for (i = 0; i < n; ++i) {
		const ACPI_TABLE_HEADER *tbl =
		    (const ACPI_TABLE_HEADER *)acpi_xsdt->TableOffsetEntry[i];
		if (i % 18 == 0)
			cputs("\n   ");
		cprintf(" %4.4s", tbl->Signature);
		if (!fadt &&
		    memcmp(tbl->Signature, fadt_sig, sizeof fadt_sig) == 0)
			fadt = (const ACPI_TABLE_FADT *)tbl;
		else if (!madt &&
		     memcmp(tbl->Signature, madt_sig, sizeof madt_sig) == 0)
			madt = (const ACPI_TABLE_MADT *)tbl;
	}
	putwch(u'\n');
	process_fadt(fadt);
	process_madt(madt);
	for (i = 0; i < n; ++i) {
		const ACPI_TABLE_HEADER *tbl =
		    (const ACPI_TABLE_HEADER *)acpi_xsdt->TableOffsetEntry[i];
		if (memcmp(tbl->Signature, hpet_sig, sizeof hpet_sig) == 0) {
			const ACPI_TABLE_HPET *hpet =
			    (const ACPI_TABLE_HPET *)tbl;
			process_hpet(hpet);
		}
	}
	process_hpet(NULL);
}

static INIT_TEXT ACPI_STATUS list_devices_cb(ACPI_HANDLE object,
    UINT32 nest_lvl, void *ctx, void **rv)
{
	ACPI_BUFFER path = { .Length = ACPI_ALLOCATE_BUFFER };
	ACPI_STATUS status = AcpiNsHandleToPathname(object, &path, FALSE);
	if (ACPI_FAILURE(status))
		panic_acpi("AcpiNsHandleToPathname", status);
	cprintf("%s ", path.Pointer);
	AcpiOsFree(path.Pointer);
	return AE_OK;
}

static INIT_TEXT void list_devices(void)
{
	ACPI_HANDLE root_h;
	ACPI_STATUS status = AcpiGetHandle(NULL, ACPI_NS_ROOT_PATH, &root_h);
	if (ACPI_FAILURE(status))
		panic_acpi("AcpiGetHandle", status);
	status = AcpiWalkNamespace(ACPI_TYPE_DEVICE, root_h, UINT_MAX,
	    list_devices_cb, NULL, NULL, NULL);
	if (ACPI_FAILURE(status))
		panic_acpi("AcpiWalkNamespace", status);
}

INIT_TEXT void acpi_init(const void *p)
{
	const char rsdp_sig[8] = "RSD PTR ";
	const ACPI_TABLE_RSDP *rsdp = (const ACPI_TABLE_RSDP *)p;
	ACPI_STATUS status;
	if (memcmp(rsdp->Signature, rsdp_sig, sizeof rsdp_sig) != 0)
		panic("ACPI RSDP has bogus signature");
	cprintf("ACPI v2 RSDP @%p:\n"
		"  OEM: %6.6s  revision: %u  RSDT: @%p  XSDT: @%p\n",
	    rsdp, rsdp->OemId, (unsigned)rsdp->Revision,
	    (void *)(uintptr_t)rsdp->RsdtPhysicalAddress,
	    (void *)rsdp->XsdtPhysicalAddress);
	acpi_rsdp = rsdp;
	acpi_xsdt = (const ACPI_TABLE_XSDT *)rsdp->XsdtPhysicalAddress;
	process_xsdt();
	status = AcpiInitializeSubsystem();
	if (ACPI_FAILURE(status))
		panic_acpi("AcpiInitializeSubsystem", status);
	status = AcpiInitializeTables(NULL, 0, TRUE);
	if (ACPI_FAILURE(status))
		panic_acpi("AcpiInitializeTables", status);
	status = AcpiLoadTables();
	if (ACPI_FAILURE(status))
		panic_acpi("AcpiLoadTables", status);
	status = AcpiEnableSubsystem(ACPI_FULL_INITIALIZATION);
	if (ACPI_FAILURE(status))
		panic_acpi("AcpiEnableSubsystem", status);
	status = AcpiInitializeObjects(ACPI_FULL_INITIALIZATION);
	if (ACPI_FAILURE(status))
		panic_acpi("AcpiInitializeObjects", status);
	list_devices();
}
