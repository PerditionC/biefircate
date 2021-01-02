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
	panic("%s: ACPI_STATUS %#" PRIx32, msg, (uint32_t)status);
}

static INIT_TEXT void process_xsdt(void)
{
	const char xsdt_sig[4] = "XSDT";
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
	}
	putwch(u'\n');
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
