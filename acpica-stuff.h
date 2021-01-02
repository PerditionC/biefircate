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

/* Definitions for working with ACPICA header files. */

#ifndef H_ACPICA_STUFF
#define H_ACPICA_STUFF

#include "truckload.h"

/* ACPICA needs these... */
#define ACPI_USE_SYSTEM_INTTYPES
#define ACPI_SYSTEM_XFACE
#define ACPI_INTERNAL_VAR_XFACE
#define ACPI_INIT_FUNCTION	INIT_TEXT
#define ACPI_USE_LOCAL_CACHE
#define ACPI_DEBUG_OUTPUT

/* Also include inline functions. */
#include "acpica-osl-inl.h"

#include "platform/acgcc.h"
#include "platform/acwin64.h"

#endif
