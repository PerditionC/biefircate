#include <efi.h>
#include <efilib.h>
#include <stddef.h>
#include <string.h>
#include "efi-stuff.h"

/* ACPICA needs these... */
#define ACPI_USE_SYSTEM_INTTYPES
#define ACPI_MACHINE_WIDTH	__INTPTR_WIDTH__
#define ACPI_SYSTEM_XFACE

#include "actypes.h"
#include "actbl.h"

static const ACPI_TABLE_XSDT *acpi_xsdt = NULL;

void process_xsdt(void)
{
	const char xsdt_sig[4] = "XSDT";
	UINT32 n, i;
	if (memcmp(acpi_xsdt->Header.Signature, xsdt_sig, sizeof xsdt_sig)
	    != 0)
		error(u"ACPI XSDT has bogus signature");
	n = (acpi_xsdt->Header.Length
	    - offsetof(ACPI_TABLE_XSDT, TableOffsetEntry)) / sizeof(UINT64);
	Output(u"  description tables:");
	for (i = 0; i < n; ++i) {
		const ACPI_TABLE_HEADER *tbl =
		    (const ACPI_TABLE_HEADER *)acpi_xsdt->TableOffsetEntry[i];
		if (i % 18 == 0)
			Output(u"\r\n   ");
		Print(u" %c%c%c%c", (CHAR16)(unsigned char)tbl->Signature[0],
				    (CHAR16)(unsigned char)tbl->Signature[1],
				    (CHAR16)(unsigned char)tbl->Signature[2],
				    (CHAR16)(unsigned char)tbl->Signature[3]);
	}
	Output(u"\r\n");
}

void process_acpi_v2_tables(void *p)
{
	const char rsdp_sig[8] = "RSD PTR ";
	const ACPI_TABLE_RSDP *rsdp = (const ACPI_TABLE_RSDP *)p;
	if (memcmp(rsdp->Signature, rsdp_sig, sizeof rsdp_sig) != 0)
		error(u"ACPI RSDP has bogus signature");
	Print(u"ACPI v2 RSDP @0x%lx:\r\n"
	       "  OEM: %c%c%c%c%c%c  revision: %u  "
		 "RSDT: @0x%x  XSDT: @0x%lx\r\n",
	    (UINT64)rsdp,
	    (CHAR16)(unsigned char)rsdp->OemId[0],
	    (CHAR16)(unsigned char)rsdp->OemId[1],
	    (CHAR16)(unsigned char)rsdp->OemId[2],
	    (CHAR16)(unsigned char)rsdp->OemId[3],
	    (CHAR16)(unsigned char)rsdp->OemId[4],
	    (CHAR16)(unsigned char)rsdp->OemId[5],
	    (UINT32)rsdp->Revision,
	    (UINT32)rsdp->RsdtPhysicalAddress,
	    (UINT64)rsdp->XsdtPhysicalAddress);
	acpi_xsdt = (const ACPI_TABLE_XSDT *)rsdp->XsdtPhysicalAddress;
	process_xsdt();
}
