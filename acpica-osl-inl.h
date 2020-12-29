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

/*
 * Inline implementations of some functions for the OS Services Layer (OSL)
 * for the ACPICA library.  These will be compiled into the libacpica.a
 * binary library file.
 */

#ifndef H_ACPICA_OSL_INL_H
#define H_ACPICA_OSL_INL_H

/*
 * Note: this file should not include ACPICA headers.  The ACPICA .c modules
 * sets certain macros, e.g. ACPI_DEFINE_EXCEPTION_TABLE, to change the way
 * the ACPICA headers behave.  If this file includes them first at this very
 * early point, things will break.
 */

#define ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsCreateSemaphore
#define AcpiOsCreateSemaphore(max_units, ini_units, p_out_handle) \
	({ \
		ACPI_HANDLE *__p = (p_out_handle); \
		!__p ? AE_BAD_PARAMETER \
		     : (*__p = (ACPI_HANDLE)1, AE_OK); \
	})

#define ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsDeleteSemaphore
#define AcpiOsDeleteSemaphore(handle)	AE_OK

#define ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsWaitSemaphore
#define AcpiOsWaitSemaphore(handle, units, timeout) AE_OK

#define ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsSignalSemaphore
#define AcpiOsSignalSemaphore(handle, units) AE_OK

#define ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsCreateLock
#define AcpiOsCreateLock(p_out_handle) \
	AcpiOsCreateSemaphore(1, 1, (p_out_handle))

#define ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsDeleteLock
#define AcpiOsDeleteLock(handle)	((void)AcpiOsDeleteSemaphore(handle))

#define ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsAcquireLock
#define AcpiOsAcquireLock(handle) \
	((void)AcpiOsWaitSemaphore((handle), 1, 0xffff), \
	 (ACPI_CPU_FLAGS)0)

#define ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsReleaseLock
#define AcpiOsReleaseLock(handle, flags) \
	((void)(flags), \
	 (void)AcpiOsSignalSemaphore((handle), 1))

#endif
