#define GNU_EFI_USE_MS_ABI

#include <efi.h>
#include <efilib.h>

extern EFI_GUID gEfiLoadedImageProtocolGuid, LegacyBootProtocol;

static void wait_and_exit(EFI_STATUS status)
{
	Output(u"press a key to exit\r\n");
	WaitForSingleEvent(ST->ConIn->WaitForKey, 0);
	Exit(status, 0, NULL);
}

static void error_with_status(IN CONST CHAR16 *msg, EFI_STATUS status)
{
	Print(u"error: %s: %d\r\n", msg, (INT32)status);
	wait_and_exit(status);
}

EFI_STATUS efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *system_table)
{
	EFI_STATUS status;
	EFI_LOADED_IMAGE_PROTOCOL *li;
	EFI_BLOCK_IO_PROTOCOL *bio;
	EFI_BLOCK_IO_MEDIA *iom;
	struct _EFI_LEGACY_BIOS_PROTOCOL *lb;
	InitializeLib(image_handle, system_table);
	Output(u"Hello world!\r\n");
	status = BS->HandleProtocol(image_handle, &gEfiLoadedImageProtocolGuid,
	    (void **)&li);
	if (EFI_ERROR(status))
		error_with_status(u"cannot get EFI_LOADED_IMAGE_PROTOCOL",
		    status);
	status = BS->HandleProtocol(li->DeviceHandle, &gEfiBlockIoProtocolGuid,
	    (void **)&bio);
	if (EFI_ERROR(status))
		error_with_status(u"cannot get EFI_BLOCK_IO_PROTOCOL", status);
	iom = bio->Media;
	Print(u"we are booted from\r\n"
	       "  media id.: 0x%x  logical partition: %s\r\n"
	       "  block size: 0x%x  I/O align: 0x%x\r\n",
	    iom->MediaId,
	    iom->LogicalPartition ? u"yes" : u"no",
	    iom->BlockSize,
	    iom->IoAlign);
	status = BS->LocateProtocol(&LegacyBootProtocol, NULL, (void **)&lb);
	if (EFI_ERROR(status))
		error_with_status(u"cannot get LEGACY_BOOT_PROTOCOL", status);
	wait_and_exit(0);
	return 0;
}
