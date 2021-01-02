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

/* Used by ACPICA (via acpica-osl.c). */
const ACPI_TABLE_RSDP *acpi_rsdp = NULL;

static const ACPI_TABLE_XSDT *acpi_xsdt = NULL;

static NORETURN void panic_acpi(const char *msg, ACPI_STATUS status)
{
	panic_with_caller(__builtin_return_address(0),
	    "%s: ACPI_STATUS %#" PRIx32, msg, (uint32_t)status);
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
	    fadt->BootFlags & 0x0001 ? "legacy-devs " : "",
	    fadt->BootFlags & 0x0002 ? "8042 " : "",
	    fadt->BootFlags & 0x0004 ? "\u00ac""VGA " : "",
	    fadt->BootFlags & 0x0008 ? "\u00ac""MSI " : "",
	    fadt->BootFlags & 0x0010 ? "\u00ac""ASPM " : "",
	    fadt->BootFlags & 0x0020 ? "\u00ac""CMOS-RTC " : "");
}

static INIT_TEXT void process_xsdt(void)
{
	const char xsdt_sig[4] = "XSDT", fadt_sig[4] = "FACP";
	const ACPI_TABLE_FADT *fadt = NULL;
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
		if (memcmp(tbl->Signature, fadt_sig, sizeof fadt_sig) == 0)
			fadt = (const ACPI_TABLE_FADT *)tbl;
	}
	putwch(u'\n');
	process_fadt(fadt);
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
		panic_acpi("AcpiInitializeSubsystem failed", status);
	status = AcpiInitializeTables(NULL, 0, TRUE);
	if (ACPI_FAILURE(status))
		panic_acpi("AcpiInitializeTables failed", status);
	status = AcpiLoadTables();
	if (ACPI_FAILURE(status))
		panic_acpi("AcpiLoadTables failed", status);
	status = AcpiEnableSubsystem(ACPI_FULL_INITIALIZATION);
	if (ACPI_FAILURE(status))
		panic_acpi("AcpiEnableSubsystem failed", status);
	status = AcpiInitializeObjects(ACPI_FULL_INITIALIZATION);
	if (ACPI_FAILURE(status))
		panic_acpi("AcpiInitializeObjects failed", status);
}
