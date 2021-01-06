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

#include <efi.h>
#include <efilib.h>
#include <string.h>
#include "efi-stuff.h"
#include "truckload.h"

INIT_TEXT EFI_STATUS efi_main(EFI_HANDLE image_handle,
			      EFI_SYSTEM_TABLE *system_table)
{
	const void *rsdp;
	InitializeLib(image_handle, system_table);
	stage1(&rsdp);
	stage2();
	stage3(rsdp);
	__builtin_unreachable();
}
