#include <efi.h>
#include <efilib.h>
#include <string.h>
#include "efi-stuff.h"
#include "loader.h"

EFI_STATUS efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *system_table)
{
	const void *rsdp;
	InitializeLib(image_handle, system_table);
	stage1(&rsdp);
	stage2(rsdp);
	__builtin_unreachable();
}
