#include <efi.h>
#include <efilib.h>
#include <string.h>
#include "efi-stuff.h"
#include "truckload.h"

extern EFI_HANDLE LibImageHandle;
extern EFI_GUID gEfiLoadedImageProtocolGuid, gEfiGlobalVariableGuid;

static BOOLEAN secure_boot_p = FALSE;
static EFI_HANDLE boot_media_handle;

static void process_efi_conf_tables(const void **p_rsdp)
{
	static const EFI_GUID Acpi20TableGuid = ACPI_20_TABLE_GUID;
	UINTN i, sct_n = ST->NumberOfTableEntries;
	void *rsdp = NULL;
	cputws(u"system configuration tables:");
	for (i = 0; i != sct_n; ++i) {
		EFI_CONFIGURATION_TABLE *cft = &ST->ConfigurationTable[i];
		EFI_GUID *vguid = &cft->VendorGuid;
		if (i % 2 == 0)
			putwch(u'\n');
		cwprintf
		   (u" | %08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
		    vguid->Data1, (UINT32)vguid->Data2, (UINT32)vguid->Data3,
		    (UINT32)vguid->Data4[0], (UINT32)vguid->Data4[1],
		    (UINT32)vguid->Data4[2], (UINT32)vguid->Data4[3],
		    (UINT32)vguid->Data4[4], (UINT32)vguid->Data4[5],
		    (UINT32)vguid->Data4[6], (UINT32)vguid->Data4[7]);
		if (!rsdp &&
		    memcmp(vguid, &Acpi20TableGuid, sizeof(EFI_GUID)) == 0)
			rsdp = cft->VendorTable;
	}
	putwch(u'\n');
	if (!rsdp)
		error(u"no ACPI v2 RSDP!");
	*p_rsdp = rsdp;
}

static void find_boot_media(void)
{
	EFI_STATUS status;
	EFI_LOADED_IMAGE_PROTOCOL *li;
	status = BS->HandleProtocol(LibImageHandle,
	    &gEfiLoadedImageProtocolGuid, (void **)&li);
	if (EFI_ERROR(status))
		error_with_status(u"cannot get EFI_LOADED_IMAGE_PROTOCOL",
		    status);
	boot_media_handle = li->DeviceHandle;
}

static void test_if_secure_boot(void)
{
	UINT8 data = 0;
	UINTN data_sz = sizeof(data);
	EFI_STATUS status = RT->GetVariable(u"SecureBoot",
	    &gEfiGlobalVariableGuid, NULL, &data_sz, &data);
	if (!EFI_ERROR(status) && data)
		secure_boot_p = TRUE;
	cwprintf(u"secure boot: %s\n", secure_boot_p ? u"yes" : u"no");
}

void stage1(const void **p_rsdp)
{
	UINTN mem_map_key;

	/* Start the frame buffer console early. */
	fb_con_init();

	/* Use UEFI boot services to figure out our environment. */
	process_efi_conf_tables(p_rsdp);
	find_boot_media();
	test_if_secure_boot();

	/*
	 * Retrieve & process the system memory map.  Then jettison UEFI boot
	 * services & start transitioning to our own services.
	 */
	mem_map_init(&mem_map_key);
	stage1_done(mem_map_key);
}
