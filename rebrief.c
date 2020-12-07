#define GNU_EFI_USE_MS_ABI

#include <efi.h>
#include <efilib.h>

extern EFI_GUID gEfiLoadedImageProtocolGuid, LegacyBootProtocol;

static void error_with_status(IN CONST CHAR16 *msg, EFI_STATUS status)
{
	Print(u"error: %s: %d\r\n", msg, (INT32)status);
	Exit(status, 0, NULL);
}

EFI_STATUS efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *system_table)
{
	EFI_STATUS status;
	EFI_LOADED_IMAGE *li;
	struct _EFI_LEGACY_BIOS_PROTOCOL *lb;
	InitializeLib(image_handle, system_table);
	Output(u"Hello world!\r\n");
	status = gBS->HandleProtocol(image_handle,
	    &gEfiLoadedImageProtocolGuid, (void **)&li);
	if (EFI_ERROR(status))
		error_with_status(u"cannot get EFI_LOADED_IMAGE", status);
	status = gBS->LocateProtocol(&LegacyBootProtocol, NULL, (void **)&lb);
	if (EFI_ERROR(status))
		error_with_status(u"cannot get LEGACY_BOOT_PROTOCOL", status);
	return 0;
}
