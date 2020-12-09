#define GNU_EFI_USE_MS_ABI

#include <efi.h>
#include <efilib.h>
#include <legacyboot.h>

typedef struct _EFI_LEGACY_BIOS_PROTOCOL EFI_LEGACY_BIOS_PROTOCOL;

extern EFI_HANDLE LibImageHandle;
extern EFI_GUID gEfiLoadedImageProtocolGuid, gEfiGlobalVariableGuid,
		LegacyBootProtocol;

static EFI_GUID gEfiLegacyBiosProtocolGuid =
    { 0xdb9a1e3d, 0x45cb, 0x4abb,
      { 0x85, 0x3b, 0xe5, 0x38, 0x7f, 0xdb, 0x2e, 0x2d } };
static BOOLEAN secure_boot_p = FALSE;

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

static void error(IN CONST CHAR16 *msg)
{
	Print(u"error: %s\r\n", msg);
	wait_and_exit(EFI_ABORTED);
}

static void process_memory_map(void)
{
	EFI_MEMORY_DESCRIPTOR *desc;
	UINTN num_entries = 0, map_key, desc_sz;
	UINT32 desc_ver;
	desc = LibMemoryMap(&num_entries, &map_key, &desc_sz, &desc_ver);
	if (!num_entries)
		error(u"cannot get memory map!");
	Output(u"memory map below 16 MiB:\r\n"
		"  start    end       type attrs\r\n");
	while (num_entries-- != 0) {
		EFI_PHYSICAL_ADDRESS start, end;
		start = desc->PhysicalStart;
		if (start > 0xffffffUL)
			continue;
		end = start + 0x1000UL * desc->NumberOfPages - 1;
		Print(u"  0x%06lx 0x%06lx%c %4u 0x%08lx\r\n", start,
		    end > 0xffffffUL ? (UINT64)0xffffffUL : end,
		    end > 0xffffffUL ? u'+' : u' ',
		    (UINT32)desc->Type, desc->Attribute);
		/* :-| */
		desc = (EFI_MEMORY_DESCRIPTOR *)((char *)desc + desc_sz);
	}
}

static EFI_BLOCK_IO_MEDIA *find_boot_media(void)
{
	EFI_STATUS status;
	EFI_LOADED_IMAGE_PROTOCOL *li;
	EFI_BLOCK_IO_PROTOCOL *bio;
	EFI_BLOCK_IO_MEDIA *iom;
	status = BS->HandleProtocol(LibImageHandle,
	    &gEfiLoadedImageProtocolGuid, (void **)&li);
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
	return iom;
}

static void test_if_secure_boot(void)
{
	UINT8 data = 0;
	UINTN data_sz = sizeof(data);
	EFI_STATUS status = RT->GetVariable(u"SecureBoot",
	    &gEfiGlobalVariableGuid, NULL, &data_sz, &data);
	if (!EFI_ERROR(status) && data)
		secure_boot_p = TRUE;
	Print(u"secure boot: %s\r\n", secure_boot_p ? u"yes" : u"no");
}

EFI_STATUS efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *system_table)
{
	EFI_STATUS status;
	EFI_BLOCK_IO_MEDIA *iom;
	EFI_LEGACY_BIOS_PROTOCOL *lbios;
	LEGACY_BOOT_INTERFACE *lbt;  /* ?!? */
	InitializeLib(image_handle, system_table);
	Output(u"Hello world!\r\n");
	process_memory_map();
	iom = find_boot_media();
	test_if_secure_boot();
	status = BS->LocateProtocol(&gEfiLegacyBiosProtocolGuid, NULL,
	    (void **)&lbios);
	if (EFI_ERROR(&status))
		error_with_status(u"cannot get EFI_LEGACY_BIOS_PROTOCOL",
		    status);
	status = BS->LocateProtocol(&LegacyBootProtocol, NULL, (void **)&lbt);
	if (EFI_ERROR(status))
		error_with_status(u"cannot get LEGACY_BOOT_PROTOCOL", status);
	wait_and_exit(0);
	return 0;
}
