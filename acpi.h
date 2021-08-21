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
#include "common.h"

/* Flags in acpi_fadt_t::iapc_boot_flags. */
#define FADT_IAPC_LEGACY_DEVS	(1 <<  0)
#define FADT_IAPC_8042		(1 <<  1)
#define FADT_IAPC_NOVGA		(1 <<  2)
#define FADT_IAPC_NOMSI		(1 <<  3)
#define FADT_IAPC_NOASPM	(1 <<  4)
#define FADT_IAPC_NORTC		(1 <<  5)

/* Structure of a Multiple APIC Description Table (MADT). */
typedef struct __attribute__((packed)) {
	acpi_header_t header;		/* header with signature "APIC" */
	uint32_t lapic_phy_addr;	/* local APIC address */
	uint32_t flags;			/* multiple APIC flags */
	char ics[];			/* interrupt controller structures */
} acpi_madt_t;

/* Flags in acpi_madt_t::flags. */
#define MADT_PCAT_COMPAT	(1 <<  0)

/* Union of ACPI table types. :-) */
typedef union __attribute__((packed)) {
	acpi_header_t header;
	acpi_xsdt_t xsdt;
	acpi_fadt_t fadt;
	acpi_madt_t madt;
} acpi_table_union_t;

/* Header of an interrupt controller structure within an MADT. */
typedef struct __attribute__((packed)) {
	uint8_t type;			/* structure type */
	uint8_t length;			/* structure length */
} acpi_madt_ic_header_t;

/* Values in acpi_madt_ic_header_t::type. */
#define MADT_IC_LAPIC		0x0	/* processor local APIC */
#define MADT_IC_IOAPIC		0x1	/* I/O APIC */
#define MADT_IC_IRQ_OVERRIDE	0x2	/* interrupt source override */
#define MADT_IC_NMI_SOURCE	0x3	/* NMI source */
#define MADT_IC_LAPIC_NMI	0x4	/* local APIC NMI */
#define MADT_IC_LAPIC_ADDR	0x5	/* local APIC address override */
#define MADT_IC_IOSAPIC		0x6	/* I/O SAPIC */
#define MADT_IC_LSAPIC		0x7	/* local SAPIC */
#define MADT_IC_PLATFORM_IRQ	0x8	/* platform interrupt source */
#define MADT_IC_LX2APIC		0x9	/* local x2APIC */
#define MADT_IC_LX2APIC_NMI	0xa	/* local x2APIC NMI */

/* Interrupt controller structure for an I/O APIC. */
typedef struct __attribute__((packed))
{
	acpi_madt_ic_header_t header;	/* header, type MADT_IC_IOAPIC */
	uint8_t id;			/* I/O APIC id. */
	uint8_t reserved;		/* reserved */
	uint32_t ioapic_phy_addr;	/* I/O APIC address */
} acpi_madt_ic_ioapic_t;

/* Interrupt controller structure for a local APIC address override. */
typedef struct __attribute__((packed))
{
	acpi_madt_ic_header_t header;	/* header, type MADT_IC_LAPIC_ADDR */
	uint8_t reserved[2];		/* reserved */
	ptr64_t lapic_phy_addr64;	/* 64-bit local APIC address */
} acpi_madt_ic_lapic_addr_t;

/* Union of interrupt controller structure types. :-) :-) */
typedef union __attribute__((packed))
{
	acpi_madt_ic_header_t header;
	acpi_madt_ic_ioapic_t ioapic;
	acpi_madt_ic_lapic_addr_t lapic_addr;
} acpi_madt_ic_union_t;

#endif
