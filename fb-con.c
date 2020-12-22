#include <efi.h>
#include <efilib.h>
#include <string.h>
#include "efi-stuff.h"
#include "loader.h"

#define VID_MIN_CHAR_WD		80
#define CHAR_PIX_WD		8
#define VID_MIN_CHAR_HT		25
#define CHAR_PIX_HT		8
#define VID_MIN_PIX_WD		(VID_MIN_CHAR_WD * CHAR_PIX_WD)
#define VID_MIN_PIX_HT		(VID_MIN_CHAR_WD * CHAR_PIX_HT)

extern EFI_GUID gEfiGraphicsOutputProtocolGuid;

static void dump_mode_info(UINT32 mode_num,
			   const EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info)
{
	Print(u"  mode 0x%x  %u", mode_num, info->HorizontalResolution);
	if (info->PixelsPerScanLine != info->HorizontalResolution)
		Print(u"{%u}", info->PixelsPerScanLine);
	Print(u"*%u  ", info->VerticalResolution);
	switch (info->PixelFormat) {
	    case PixelRedGreenBlueReserved8BitPerColor:
		Output(u"RGBX8888");  break;
	    case PixelBlueGreenRedReserved8BitPerColor:
		Output(u"BGRX8888");  break;
	    case PixelBitMask:
		Print(u"RGBX %x/%x/%x/%x",
		      info->PixelInformation.RedMask,
		      info->PixelInformation.GreenMask,
		      info->PixelInformation.BlueMask,
		      info->PixelInformation.ReservedMask);
		break;
	    default:
		Output(u"BLT-only");
	}
	Print(u"\r\n");
}

static void wait_1_second(void)
{
	EFI_EVENT timer_event;
	UINTN index;
	EFI_STATUS status =
	    BS->CreateEvent(EVT_TIMER, 0, NULL, NULL, &timer_event);
	if (EFI_ERROR(status))
		return;
	status = BS->SetTimer(timer_event, TimerRelative, 10000000ULL);
	if (EFI_ERROR(status))
		return;
	BS->WaitForEvent(1, &timer_event, &index);
}

void init_fb_con(void)
{
	UINT32 mode_num, max_mode_num, best_mode_num;
	EFI_GRAPHICS_OUTPUT_MODE_INFORMATION best_info = { };
	EFI_GRAPHICS_OUTPUT_PROTOCOL *gro;
	EFI_STATUS status = BS->LocateProtocol(&gEfiGraphicsOutputProtocolGuid,
	    NULL, (void **)&gro);
	if (EFI_ERROR(status))
		error_with_status(u"cannot get EFI_GRAPHICS_OUTPUT_PROTOCOL",
		    status);
	Print(u"current UEFI graphics mode: 0x%x\r\n"
	       "available framebuffer modes:\r\n", gro->Mode->Mode);
	best_mode_num = max_mode_num = gro->Mode->MaxMode;
	for (mode_num = 0; mode_num < max_mode_num; ++mode_num) {
		EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info;
		UINTN info_size;
		status = gro->QueryMode(gro, mode_num, &info_size, &info);
		if (EFI_ERROR(status))
			continue;
		if (info->HorizontalResolution < VID_MIN_PIX_WD ||
		    info->VerticalResolution < VID_MIN_PIX_HT)
			continue;
		dump_mode_info(mode_num, info);
		switch (info->PixelFormat) {
		    case PixelRedGreenBlueReserved8BitPerColor:
		    case PixelBlueGreenRedReserved8BitPerColor:
		    case PixelBitMask:
			break;
		    default:
			continue;
		}
		if (best_mode_num == max_mode_num) {
			best_mode_num = mode_num;
			best_info = *info;
			continue;
		}
		if (info->HorizontalResolution <
		    best_info.HorizontalResolution) {
			best_mode_num = mode_num;
			best_info = *info;
			continue;
		}
		if (info->HorizontalResolution ==
		    best_info.HorizontalResolution &&
		    info->VerticalResolution >
		    best_info.VerticalResolution) {
			best_mode_num = mode_num;
			best_info = *info;
			continue;
		}
	}
	if (best_mode_num == max_mode_num)
		error(u"no suitable graphics mode to switch to!");
	Print(u"switching to mode 0x%x in 5", best_mode_num);
	wait_1_second();
	Output(u" 4");
	wait_1_second();
	Output(u" 3");
	wait_1_second();
	Output(u" 2");
	wait_1_second();
	Output(u" 1");
	wait_1_second();
	status = gro->SetMode(gro, best_mode_num);
	if (EFI_ERROR(status)) {
		Output(u"\r\n");
		error_with_status(u"cannot set graphics mode", status);
	}
	/* TODO: replace ST->ConOut with own implementation */
}
