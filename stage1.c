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
#include <string.h>
#include "efi-stuff.h"
#include "truckload.h"

extern EFI_HANDLE LibImageHandle;
extern EFI_GUID gEfiLoadedImageProtocolGuid, gEfiGlobalVariableGuid;

static BOOLEAN secure_boot_p = FALSE;
static EFI_HANDLE boot_media_handle;

static INIT_TEXT void process_efi_conf_tables(const void **p_rsdp)
{
	static const EFI_GUID Acpi20TableGuid = ACPI_20_TABLE_GUID;
	UINTN i, sct_n = ST->NumberOfTableEntries;
	void *rsdp = NULL;
	cputs("system configuration tables:");
	for (i = 0; i != sct_n; ++i) {
		EFI_CONFIGURATION_TABLE *cft = &ST->ConfigurationTable[i];
		EFI_GUID *vguid = &cft->VendorGuid;
		if (i % 2 == 0)
			putwch(u'\n');
		cprintf
		   (" | %08" PRIx64 "-%04" PRIx16 "-%04" PRIx16
		    "-%02x%02x-%02x%02x%02x%02x%02x%02x",
		    vguid->Data1, vguid->Data2, vguid->Data3,
		    (unsigned)vguid->Data4[0], (unsigned)vguid->Data4[1],
		    (unsigned)vguid->Data4[2], (unsigned)vguid->Data4[3],
		    (unsigned)vguid->Data4[4], (unsigned)vguid->Data4[5],
		    (unsigned)vguid->Data4[6], (unsigned)vguid->Data4[7]);
		if (!rsdp &&
		    memcmp(vguid, &Acpi20TableGuid, sizeof(EFI_GUID)) == 0)
			rsdp = cft->VendorTable;
	}
	putwch(u'\n');
	if (!rsdp)
		panic("no ACPI v2 RSDP!");
	*p_rsdp = rsdp;
}

static INIT_TEXT void find_boot_media(void)
{
	EFI_STATUS status;
	EFI_LOADED_IMAGE_PROTOCOL *li;
	status = BS->HandleProtocol(LibImageHandle,
	    &gEfiLoadedImageProtocolGuid, (void **)&li);
	if (EFI_ERROR(status))
		panic_efi("cannot get EFI_LOADED_IMAGE_PROTOCOL", status);
	boot_media_handle = li->DeviceHandle;
}

static INIT_TEXT void test_if_secure_boot(void)
{
	UINT8 data = 0;
	UINTN data_sz = sizeof(data);
	EFI_STATUS status = RT->GetVariable(u"SecureBoot",
	    &gEfiGlobalVariableGuid, NULL, &data_sz, &data);
	if (!EFI_ERROR(status) && data)
		secure_boot_p = TRUE;
	cprintf("secure boot: %s\n", secure_boot_p ? "yes" : "no");
}

INIT_TEXT void stage1(const void **p_rsdp, uintptr_t *mapped_mem_end)
{
	UINTN mem_map_key;

	/* Start the frame buffer console early. */
	fb_con_init();

	/* Initialize our internal dynamic memory heap. */
	mem_heap_init();

	/* Use UEFI boot services to figure out our environment. */
	process_efi_conf_tables(p_rsdp);
	find_boot_media();
	test_if_secure_boot();

	/*
	 * Retrieve & process the system memory map.  Then jettison UEFI boot
	 * services & start transitioning to our own services.
	 */
	mem_map_init(&mem_map_key);
	stage1_done(mem_map_key, mapped_mem_end);
}
