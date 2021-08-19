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

/* ACPI stuff. */

#ifndef H_ACPI
#define H_ACPI

#include <acpispec/tables.h>

/* Flags in acpi_fadt_t::iapc_boot_flags. */
#define FADT_IAPC_LEGACY_DEVS	(1 <<  0)
#define FADT_IAPC_8042		(1 <<  1)
#define FADT_IAPC_NOVGA		(1 <<  2)
#define FADT_IAPC_NOMSI		(1 <<  3)
#define FADT_IAPC_NOASPM	(1 <<  4)
#define FADT_IAPC_NORTC		(1 <<  5)

typedef struct __attribute__((packed)) {
	acpi_header_t header;
	uint32_t lapic_phy_addr;	/* local APIC address */
	uint32_t flags;			/* multiple APIC flags */
	char ics[];			/* interrupt controller structures */
} acpi_madt_t;

/* Flags in acpi_madt_t::flags. */
#define MADT_PCAT_COMPAT	(1 <<  0)

#endif
