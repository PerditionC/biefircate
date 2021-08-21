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

#include <string.h>
#include "acpi.h"
#include "stage1/stage1.h"

static void acpi_process_fadt(acpi_fadt_t *fadt)
{
	Print(u"  FADT: @0x%lx\r\n", fadt);
	if (fadt->header.length < offsetof(acpi_fadt_t, iapc_boot_flags) +
				  sizeof(fadt->iapc_boot_flags))
		error(u"FADT too small");
	if ((fadt->iapc_boot_flags & FADT_IAPC_NOVGA) != 0)
		warn(u"FADT: no VGA h/w");
	if ((fadt->iapc_boot_flags & FADT_IAPC_NORTC) != 0)
		error(u"FADT: no CMOS RTC");
}

static void acpi_process_madt(acpi_madt_t *madt)
{
	Print(u"  MADT: @0x%lx\r\n", madt);
	if ((madt->flags & MADT_PCAT_COMPAT) == 0)
		error(u"MADT: no 8259");
}

void acpi_init(acpi_xsdp_t *rsdp)
{
	static const char expect_rsdp_sig[8] = "RSD PTR ",
	    expect_xsdt_sig[4] = "XSDT",
	    fadt_sig[4] = "FACP", madt_sig[4] = "APIC";
	acpi_xsdt_t *xsdt;
	bdat_rsdp_t *bd_rsdp;
	uint32_t sz;
	size_t num_tabs, i;
	/* Do some quick checks on the RSDP. */
	Print(u"ACPI 2+ RSDP: @0x%lx", rsdp);
	if (memcmp(rsdp->signature, expect_rsdp_sig, 8) != 0)
		error(u"RSDP has bad sig.");
	if (rsdp->revision < 2)
		error(u"RSDP revision too old");
	sz = rsdp->length;
	if (sz < sizeof(acpi_xsdp_t))
		error(u"RSDP too small");
	if (compute_cksum(rsdp, offsetof(acpi_xsdp_t, length)) != 0 ||
	    compute_cksum(rsdp, sz) != 0)
		error(u"RSDP has bad checksums");
	/* Do some quick checks on the XSDT. */
	xsdt = (acpi_xsdt_t *)rsdp->xsdt;
	Print(u"  XSDT: @0x%lx\r\n", xsdt);
	if (memcmp(xsdt->header.signature, expect_xsdt_sig, 4) != 0)
		error(u"XSDT has bad sig.");
	sz = xsdt->header.length;
	if (sz < sizeof(acpi_header_t) + sizeof(uint64_t))
		error(u"XSDT too small");
	if (compute_cksum(xsdt, sz) != 0)
		error(u"XSDT has bad checksum");
	/* Go through the tables in the XSDT. */
	num_tabs = (sz - sizeof(acpi_header_t)) / sizeof(uint64_t);
	for (i = 0; i < num_tabs; ++i) {
		acpi_table_union_t *tab =
		    (acpi_table_union_t *)xsdt->tables[i];
		if (memcmp(tab->header.signature, fadt_sig, 4) == 0)
			acpi_process_fadt(&tab->fadt);
		else if (memcmp(tab->header.signature, madt_sig, 4) == 0)
			acpi_process_madt(&tab->madt);
	}
	/* Add a boot parameter for the RSDP. */
	bd_rsdp = bparm_add(BP_RSDP, sizeof(bdat_rsdp_t));
	bd_rsdp->rsdp_phy_addr = rsdp;
	bd_rsdp->rsdp_sz = sz;
}
