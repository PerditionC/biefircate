/*
 * Copyright (c) 2020 TK Chia
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

/* OS Services Layer (OSL) for the ACPICA library. */

#include <efi.h>
#include <efilib.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include "efi-stuff.h"
#include "truckload.h"
#include "acpica-stuff.h"

/* ACPICA header files. */
#include "actypes.h"
#include "acexcep.h"
#include "actbl.h"
#include "platform/acwin64.h"
#include "acpiosxf.h"

extern const ACPI_TABLE_RSDP *acpi_rsdp;

ACPI_STATUS AcpiOsInitialize(void)
{
	return AE_OK;
}

ACPI_STATUS AcpiOsTerminate(void)
{
	return AE_OK;
}

ACPI_PHYSICAL_ADDRESS AcpiOsGetRootPointer(void)
{
	return (ACPI_PHYSICAL_ADDRESS)acpi_rsdp;
}
ACPI_STATUS AcpiOsPredefinedOverride(const ACPI_PREDEFINED_NAMES *obj,
				     ACPI_STRING *new_val)
{
	if (!obj || !new_val)
		return AE_BAD_PARAMETER;
	*new_val = NULL;
	return AE_OK;
}

ACPI_STATUS AcpiOsTableOverride(ACPI_TABLE_HEADER *old_tab,
				ACPI_TABLE_HEADER **new_tab)
{
	if (!old_tab || !new_tab)
		return AE_BAD_PARAMETER;
	*new_tab = NULL;
	return AE_OK;
}

ACPI_STATUS AcpiOsPhysicalTableOverride(ACPI_TABLE_HEADER *old_tab,
					ACPI_PHYSICAL_ADDRESS *new_addr,
					UINT32 *new_size)
{
	if (!old_tab || !new_addr || !new_size)
		return AE_BAD_PARAMETER;
	*new_addr = 0;
	*new_size = 0;
	return AE_OK;
}

void *AcpiOsAllocate(ACPI_SIZE size)
{
	return mem_heap_alloc(size);
}

void AcpiOsFree(void *p)
{
	return mem_heap_free(p);
}

void AcpiOsSleep(UINT64 ms)
{
	/* FIXME */
	panic_with_caller(__builtin_return_address(0), "AcpiOsSleep");
}

void AcpiOsStall(UINT32 us)
{
	/* FIXME */
	panic_with_caller(__builtin_return_address(0), "AcpiOsStall");
}

void AcpiOsWaitEventsComplete(void)
{
	/* nothing to do */
}

void *AcpiOsMapMemory(ACPI_PHYSICAL_ADDRESS addr, ACPI_SIZE size)
{
	paging_extend(addr + size);
	return (void *)addr;
}

void AcpiOsUnmapMemory(void *addr, ACPI_SIZE size)
{
	/* nothing to do */
}

ACPI_STATUS AcpiOsInstallInterruptHandler(UINT32 int_lvl,
    ACPI_OSD_HANDLER handler, void *context)
{
	/* FIXME */
	warn("AcpiOsInstallInterruptHandler %#" PRIx32 " %p %p",
	    int_lvl, handler, context);
	return AE_OK;
}

ACPI_STATUS AcpiOsRemoveInterruptHandler(UINT32 int_num,
    ACPI_OSD_HANDLER handler)
{
	/* FIXME */
	panic_with_caller(__builtin_return_address(0),
	    "AcpiOsRemoveInterruptHandler");
	return AE_OK;
}

ACPI_STATUS AcpiOsReadMemory(ACPI_PHYSICAL_ADDRESS address,
			     UINT64 *value, UINT32 width)
{
	if (!value)
		return AE_BAD_PARAMETER;
	switch (width) {
	    case 8:
		*value = *(volatile uint8_t *)address;
		break;
	    case 16:
		*value = *(volatile uint16_t *)address;
		break;
	    case 32:
		*value = *(volatile uint32_t *)address;
		break;
	    case 64:
		*value = *(volatile uint64_t *)address;
		break;
	    default:
		return AE_BAD_PARAMETER;
	}
	return AE_OK;
}

ACPI_STATUS AcpiOsWriteMemory(ACPI_PHYSICAL_ADDRESS address,
			      UINT64 value, UINT32 width)
{
	switch (width) {
	    case 8:
		*(volatile uint8_t *)address = (uint8_t)value;
		break;
	    case 16:
		*(volatile uint16_t *)address = (uint16_t)value;
		break;
	    case 32:
		*(volatile uint32_t *)address = (uint32_t)value;
		break;
	    case 64:
		*(volatile uint64_t *)address = value;
		break;
	    default:
		return AE_BAD_PARAMETER;
	}
	return AE_OK;
}

ACPI_STATUS AcpiOsReadPort(ACPI_IO_ADDRESS port, UINT32 *value, UINT32 width)
{
	if (!value || port > 0xffffULL)
		return AE_BAD_PARAMETER;
	switch (width) {
	    case 8:
		*value = inp((uint16_t)port);
		break;
	    case 16:
		*value = inpw((uint16_t)port);
		break;
	    case 32:
		*value = inpd((uint16_t)port);
		break;
	    default:
		return AE_BAD_PARAMETER;
	}
	return AE_OK;
}

ACPI_STATUS AcpiOsWritePort(ACPI_IO_ADDRESS port, UINT32 value, UINT32 width)
{
	if (port > 0xffffULL)
		return AE_BAD_PARAMETER;
	switch (width) {
	    case 8:
		outp((uint16_t)port, (uint8_t)value);
		break;
	    case 16:
		outpw((uint16_t)port, (uint16_t)value);
		break;
	    case 32:
		outpd((uint16_t)port, (uint32_t)value);
		break;
	    default:
		return AE_BAD_PARAMETER;
	}
	return AE_OK;
}

ACPI_STATUS AcpiOsReadPciConfiguration(ACPI_PCI_ID *pci_id, UINT32 reg_num,
    UINT64 *value, UINT32 width)
{
	const uint16_t ADDR_PORT = 0x0cf8, DATA_PORT = 0x0cfc;
	unsigned bus, device, function, shift = 0;
	UINT64 v = 0;
	UINT32 address;
	if (!value)
		return AE_BAD_PARAMETER;
	switch (width) {
	    case 8:
	    case 16:
	    case 32:
	    case 64:
		break;
	    default:
		return AE_BAD_PARAMETER;
	}
	width /= 8;
	if (reg_num > 0xff || reg_num + width > 0x100)
		return AE_BAD_PARAMETER;
	bus = pci_id->Bus;
	device = pci_id->Device;
	function = pci_id->Function;
	if (bus > 0xff || device > 0x1f || function > 7)
		return AE_BAD_PARAMETER;
	address = (UINT32)1 << 31 | (UINT32)bus << 16 |
		  (UINT32)device << 11 | (UINT32)function << 8;
	/* FIXME: this is slow and stupid. */
	while (width) {
		outpd(ADDR_PORT, address | ((UINT8)reg_num & ~(UINT8)3));
		v |= (UINT64)inp(DATA_PORT + (UINT16)reg_num % 4U) << shift;
		shift += 8;
		++reg_num;
		--width;
	}
	*value = v;
	return AE_OK;
}

ACPI_STATUS AcpiOsWritePciConfiguration(ACPI_PCI_ID *pci_id, UINT32 reg_num,
    UINT64 value, UINT32 width)
{
	const uint16_t ADDR_PORT = 0x0cf8, DATA_PORT = 0x0cfc;
	unsigned bus, device, function;
	UINT32 address;
	switch (width) {
	    case 8:
	    case 16:
	    case 32:
	    case 64:
		break;
	    default:
		return AE_BAD_PARAMETER;
	}
	width /= 8;
	if (reg_num > 0xff || reg_num + width > 0x100)
		return AE_BAD_PARAMETER;
	bus = pci_id->Bus;
	device = pci_id->Device;
	function = pci_id->Function;
	if (bus > 0xff || device > 0x1f || function > 7)
		return AE_BAD_PARAMETER;
	address = (UINT32)1 << 31 | (UINT32)bus << 16 |
		  (UINT32)device << 11 | (UINT32)function << 8;
	while (width) {
		outpd(ADDR_PORT, address | ((UINT8)reg_num & ~(UINT8)3));
		outp(DATA_PORT + (UINT16)reg_num % 4U, (UINT8)value);
		value >>= 8;
		++reg_num;
		--width;
	}
	return AE_OK;
}

void ACPI_INTERNAL_VAR_XFACE AcpiOsPrintf(const char *fmt, ...)
{
	enum COLORS old_colour = textcolor(BROWN);
	va_list ap;
	va_start(ap, fmt);
	vcprintf(fmt, ap);
	va_end(ap);
	textcolor(old_colour);
}

void AcpiOsVprintf(const char *fmt, va_list ap)
{
	enum COLORS old_colour = textcolor(BROWN);
	vcprintf(fmt, ap);
	textcolor(old_colour);
}

UINT64 AcpiOsGetTimer(void)
{
	/* FIXME */
	warn("AcpiOsGetTimer");
	return 0;
}

ACPI_STATUS AcpiOsSignal(UINT32 function, void *info)
{
	/* FIXME */
	panic_with_caller(__builtin_return_address(0), "AcpiOsSignal");
	return AE_OK;
}
