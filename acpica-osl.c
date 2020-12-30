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

void *AcpiOsMapMemory(ACPI_PHYSICAL_ADDRESS addr, ACPI_SIZE size)
{
	return (void *)addr;
}

void AcpiOsUnmapMemory(void *addr, ACPI_SIZE size)
{
	/* nothing to do */
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

void ACPI_INTERNAL_VAR_XFACE AcpiOsPrintf(const char *fmt, ...)
{
}

void AcpiOsVprintf(const char *fmt, va_list ap)
{
}
