/*
 * Copyright (c) 2020--2021 TK Chia
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of the developer(s) nor the names of its
 *     contributors may be used to endorse or promote products derived from
 *     this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define GNU_EFI_USE_MS_ABI

#include <efi.h>
#include <efilib.h>
#include <stdbool.h>
#include <string.h>
#include "elf.h"
#ifdef XV6_COMPAT
#   include "mp.h"
#endif

extern EFI_HANDLE LibImageHandle;
extern EFI_GUID gEfiLoadedImageProtocolGuid;

static EFI_HANDLE boot_media_handle;

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

static void write_dump(EFI_FILE_PROTOCOL *out, EFI_FILE_PROTOCOL *vol,
		       UINTN size, void *buf)
{
	UINTN write_size = size;
	EFI_STATUS status = out->Write(out, &write_size, buf);
	if (EFI_ERROR(status)) {
		out->Close(out);
		vol->Close(vol);
		error_with_status(u"cannot write dump file", status);
	}
	if (write_size != size) {
		out->Close(out);
		vol->Close(vol);
		error(u"short write to dump file");
	}
}

static void dump_rom(CHAR16 *name, UINTN size, void *buf)
{
	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs;
	EFI_FILE_PROTOCOL *vol, *out;
	EFI_STATUS status = BS->HandleProtocol(boot_media_handle,
	    &gEfiSimpleFileSystemProtocolGuid, (void **)&fs);
	if (EFI_ERROR(status))
		error_with_status(u"cannot get "
		    "EFI_SIMPLE_FILE_SYSTEM_PROTOCOL", status);
	status = fs->OpenVolume(fs, &vol);
	if (EFI_ERROR(status))
		error_with_status(u"cannot get EFI_FILE_PROTOCOL", status);
	status = vol->Open(vol, &out, name, EFI_FILE_MODE_READ, 0);
	if (!EFI_ERROR(status)) {
		status = out->Delete(out);
		if (EFI_ERROR(status))
			error_with_status(u"cannot delete pre-existing "
					   "dump file", status);
	}
	/*
	 * (UEFI insists that EFI_FILE_MODE_WRITE must be accompanied by
	 * EFI_FILE_MODE_READ, even if we will never read the file.)  :-|
	 */
	status = vol->Open(vol, &out, name, EFI_FILE_MODE_READ |
	    EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0);
	if (EFI_ERROR(status)) {
		vol->Close(vol);
		error_with_status(u"cannot create dump file", status);
	}
	write_dump(out, vol, size, buf);
	out->Close(out);
	vol->Close(vol);
}

static bool process_one_pci_io(EFI_PCI_IO_PROTOCOL *io)
{
	UINTN seg, bus, dev, fn;
	enum { MAX_ID_LEN = 8 + 1 + 8 + 1 + 8 + 1 + 8 + 1 + 9 };
	CHAR16 name[MAX_ID_LEN + sizeof(u".dump") / sizeof(CHAR16)];
	UINT32 pci_id;
	EFI_STATUS status;
	if (!io->RomSize)
		return false;
	status = io->GetLocation(io, &seg, &bus, &dev, &fn);
	if (EFI_ERROR(status))
		error_with_status(u"cannot get PCI ctrlr. locn.", status);
	status = io->Pci.Read(io, EfiPciIoWidthUint32, 0, 1, &pci_id);
	if (EFI_ERROR(status))
		error_with_status(u"cannot read PCI conf. sp.", status);
	UnicodeSPrint(name, sizeof name, u"%04x.%02x.%02x.%02x_%04x.%04x.dump",
	    seg, bus, dev, fn,
	    (UINT32)(pci_id & 0xffffU), (UINT32)(pci_id >> 16));
	Print(u"dumping PCI ROM to %s...", name);
	dump_rom(name, io->RomSize, io->RomImage);
	Output(u" done\r\n");
	return true;
}

static void dump_pci_roms(void)
{
	EFI_HANDLE *handles;
	UINTN num_handles, idx;
	bool got_rom = false;
	EFI_STATUS status = LibLocateHandle(ByProtocol,
	    &gEfiPciIoProtocolGuid, NULL, &num_handles, &handles);
	if (EFI_ERROR(status))
		error_with_status(u"no PCI devices found", status);
	for (idx = 0; idx < num_handles; ++idx) {
		EFI_HANDLE handle = handles[idx];
		EFI_PCI_IO_PROTOCOL *io;
		status = BS->HandleProtocol(handle,
		    &gEfiPciIoProtocolGuid, (void **)&io);
		if (EFI_ERROR(status))
			error_with_status(u"cannot get EFI_PCI_IO_PROTOCOL",
			    status);
		got_rom |= process_one_pci_io(io);
	}
	if (!got_rom)
		Output(u"no PCI option ROMs to dump!\r\n");
}

#define OPT_ROM_OUT	u"0x000C0000.dump"

static void dump_opt_rom_area(void)
{
	EFI_PHYSICAL_ADDRESS start = 0xc0000ULL, end = 0x100000ULL;
	Output(u"dumping option ROM area to " OPT_ROM_OUT "...");
	dump_rom(OPT_ROM_OUT, (UINTN)(end - start), (void *)start);
	Output(u" done\r\n");
}

static void dump_fv_area(void)
{
	enum { CHUNK = 0x10000UL };
	EFI_PHYSICAL_ADDRESS start = 0xff800000ULL, end = 0x100000000ULL;
	static unsigned char buf[CHUNK];
	Output(u"dumping firmware volume area...");
	while (start < end) {
		CHAR16 name[2 + 16 + sizeof(u".dump") / sizeof(CHAR16)];
		UnicodeSPrint(name, sizeof name, u"0x%08lx.dump", start);
		memcpy(buf, (void *)start, CHUNK);
		dump_rom(name, CHUNK, buf);
		Output(u".");
		start += CHUNK;
	}
	Output(u" done\r\n");
}

EFI_STATUS efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *system_table)
{
	InitializeLib(image_handle, system_table);
	Output(u".:. ROM dumper " VERSION " .:.\r\n");
	find_boot_media();
	dump_pci_roms();
	dump_opt_rom_area();
	dump_fv_area();
	wait_and_exit(0);
	return 0;
}
