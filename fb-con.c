#include <efi.h>
#include <efilib.h>
#include <inttypes.h>
#include <stdarg.h>
#include <string.h>
#include "efi-stuff.h"
#include "loader.h"
#include "font-default.h"

#if FONT_DEFAULT_WIDTH != 8
#   error "unsupported font width"
#endif

#define VID_MIN_TEXT_WIDTH	100
#define VID_MIN_TEXT_HEIGHT	30
#define VID_MIN_PIX_WIDTH	(VID_MIN_TEXT_WIDTH * FONT_DEFAULT_WIDTH)
#define VID_MIN_PIX_HEIGHT	(VID_MIN_TEXT_HEIGHT * FONT_DEFAULT_HEIGHT)

extern EFI_GUID gEfiGraphicsOutputProtocolGuid;

typedef struct __attribute__((packed)) {
	uint8_t a8_0, a8_1, a8_2;
} v3qi_t;

typedef volatile union {
	uint8_t a8[1];
	uint16_t a16[1];
	v3qi_t a24[1];
	uint32_t a32[1];
} fb_t;

static EFI_STATUS proto_output_string(IN EFI_SIMPLE_TEXT_OUT_PROTOCOL *,
				      IN CHAR16 *);

static EFI_SIMPLE_TEXT_OUT_PROTOCOL our_txop, *orig_txop = NULL;
static EFI_GRAPHICS_OUTPUT_PROTOCOL *gfxop;
static UINT32 orig_mode;
static UINT32 text_width, text_height, pix_width, pix_height,
	      red_mask, green_mask, blue_mask, curr_bg = 0, curr_fg = 0,
	      pixel_octets;
static fb_t *frame_buf;
static unsigned curr_row = 0, curr_col = 0;

static void splash(void)
{
	Output(u".:. " PACKAGE_NAME " " PACKAGE_VERSION " .:.\r\n");
}

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

static EFI_STATUS EFIAPI proto_output_string
    (IN EFI_SIMPLE_TEXT_OUT_PROTOCOL *this, IN CHAR16 *string)
{
	cputws(string);
	return 0;
}

/*
 * Replace ST->ConOut (or parts of it) with our own implementation.  Remember
 * to back up the old ST->ConOut so that we can restore it if this loader
 * fails.
 */
static void replace_con_out(void)
{
	if (ST->ConOut) {
		orig_txop = ST->ConOut;
		our_txop = *ST->ConOut;
	}
	our_txop.OutputString = proto_output_string;
	ST->ConOut = &our_txop;
}

static const uint8_t *find_glyph(char16_t ch)
{
	char16_t lo = 0, hi = FONT_DEFAULT_GLYPHS;
	while (lo < hi - 1) {
		char16_t mid = (lo + hi) / 2;
		if (ch >= font_default_code_points[mid])
			lo = mid;
		else
			hi = mid;
	}
	if (font_default_code_points[lo] != ch)
		return NULL;
	return font_default_data[lo];
}

static void fb_con_nl(void)
{
	curr_col = 0;
	if (curr_row < text_height - 1) {
		++curr_row;
		return;
	}
}

__attribute__((optimize("unroll-loops")))
static void putwch_default(char16_t ch)
{
	const uint8_t *glyph = find_glyph(ch);
	unsigned gly_r;
	size_t offset;
	glyph = find_glyph(ch);
	if (!glyph)
		return;
	offset = (size_t)curr_row * pix_width * FONT_DEFAULT_HEIGHT
		 + (size_t)curr_col * FONT_DEFAULT_WIDTH;
	switch (pixel_octets) {
	    case 1:
		{
			uint8_t fg = (uint8_t)curr_fg, bg = (uint8_t)curr_bg;
			volatile uint8_t *cell = &frame_buf->a8[offset];
			for (gly_r = 0; gly_r < FONT_DEFAULT_HEIGHT; ++gly_r) {
				const uint8_t bits = glyph[gly_r];
				cell[0] = bits & 0x80 ? fg : bg;
				cell[1] = bits & 0x40 ? fg : bg;
				cell[2] = bits & 0x20 ? fg : bg;
				cell[3] = bits & 0x10 ? fg : bg;
				cell[4] = bits & 0x08 ? fg : bg;
				cell[5] = bits & 0x04 ? fg : bg;
				cell[6] = bits & 0x02 ? fg : bg;
				cell[7] = bits & 0x01 ? fg : bg;
				cell += pix_width;
			}
		}
		break;
	    case 2:
		{
			uint16_t fg = (uint16_t)curr_fg,
				 bg = (uint16_t)curr_bg;
			volatile uint16_t *cell = &frame_buf->a16[offset];
			for (gly_r = 0; gly_r < FONT_DEFAULT_HEIGHT; ++gly_r) {
				const uint8_t bits = glyph[gly_r];
				cell[0] = bits & 0x80 ? fg : bg;
				cell[1] = bits & 0x40 ? fg : bg;
				cell[2] = bits & 0x20 ? fg : bg;
				cell[3] = bits & 0x10 ? fg : bg;
				cell[4] = bits & 0x08 ? fg : bg;
				cell[5] = bits & 0x04 ? fg : bg;
				cell[6] = bits & 0x02 ? fg : bg;
				cell[7] = bits & 0x01 ? fg : bg;
				cell += pix_width;
			}
		}
		break;
	    case 3:
		{
			v3qi_t fg = { curr_fg, curr_fg >> 8, curr_fg >> 16 },
			       bg = { curr_bg, curr_bg >> 8, curr_bg >> 16 };
			volatile v3qi_t *cell = &frame_buf->a24[offset];
			for (gly_r = 0; gly_r < FONT_DEFAULT_HEIGHT; ++gly_r) {
				const uint8_t bits = glyph[gly_r];
				cell[0] = bits & 0x80 ? fg : bg;
				cell[1] = bits & 0x40 ? fg : bg;
				cell[2] = bits & 0x20 ? fg : bg;
				cell[3] = bits & 0x10 ? fg : bg;
				cell[4] = bits & 0x08 ? fg : bg;
				cell[5] = bits & 0x04 ? fg : bg;
				cell[6] = bits & 0x02 ? fg : bg;
				cell[7] = bits & 0x01 ? fg : bg;
				cell += pix_width;
			}
		}
		break;
	    default:
		{
			uint32_t fg = curr_fg, bg = curr_bg;
			volatile uint32_t *cell = &frame_buf->a32[offset];
			for (gly_r = 0; gly_r < FONT_DEFAULT_HEIGHT; ++gly_r) {
				const uint8_t bits = glyph[gly_r];
				cell[0] = bits & 0x80 ? fg : bg;
				cell[1] = bits & 0x40 ? fg : bg;
				cell[2] = bits & 0x20 ? fg : bg;
				cell[3] = bits & 0x10 ? fg : bg;
				cell[4] = bits & 0x08 ? fg : bg;
				cell[5] = bits & 0x04 ? fg : bg;
				cell[6] = bits & 0x02 ? fg : bg;
				cell[7] = bits & 0x01 ? fg : bg;
				cell += pix_width;
			}
		}
		break;
	}
	++curr_col;
	if (curr_col >= text_width)
		fb_con_nl();
}

static void putwch(char16_t ch)
{
	switch (ch) {
	    case u'\r':
		curr_col = 0;
		break;
	    case u'\n':
		fb_con_nl();
		break;
	    default:
		putwch_default(ch);
	}
}

/* Initialize the frame buffer console. */
void init_fb_con(void)
{
	UINT32 mode_num, max_mode_num, best_mode_num;
	EFI_GRAPHICS_OUTPUT_MODE_INFORMATION best_info = { };
	EFI_STATUS status;
	UINT32 all_mask;

	/* Find a good graphics mode to switch to. */
	splash();
	status = BS->LocateProtocol(&gEfiGraphicsOutputProtocolGuid,
	    NULL, (void **)&gfxop);
	if (EFI_ERROR(status))
		error_with_status(u"cannot get EFI_GRAPHICS_OUTPUT_PROTOCOL",
		    status);
	orig_mode = gfxop->Mode->Mode;
	Print(u"current UEFI graphics mode: 0x%x\r\n"
	       "available framebuffer modes:\r\n", orig_mode);
	best_mode_num = max_mode_num = gfxop->Mode->MaxMode;
	for (mode_num = 0; mode_num < max_mode_num; ++mode_num) {
		EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info;
		UINTN info_size;
		status = gfxop->QueryMode(gfxop, mode_num, &info_size, &info);
		if (EFI_ERROR(status))
			continue;
		if (info->HorizontalResolution < VID_MIN_PIX_WIDTH ||
		    info->VerticalResolution < VID_MIN_PIX_HEIGHT)
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

	/* Switch to the new graphics mode. */
	Print(u"switching to mode 0x%x in 3", best_mode_num);
	wait_1_second();
	Output(u" 2");
	wait_1_second();
	Output(u" 1");
	wait_1_second();
	status = gfxop->SetMode(gfxop, best_mode_num);
	if (EFI_ERROR(status)) {
		Output(u"\r\n");
		error_with_status(u"cannot set graphics mode", status);
	}

	/* Set things up for frame buffer console output. */
	pix_width = best_info.PixelsPerScanLine;
	pix_height = best_info.VerticalResolution;
	text_width = pix_width / FONT_DEFAULT_WIDTH;
	text_height = pix_height / FONT_DEFAULT_HEIGHT;
	frame_buf = (fb_t *)gfxop->Mode->FrameBufferBase;
	switch (best_info.PixelFormat) {
	    case PixelRedGreenBlueReserved8BitPerColor:
		red_mask   = 0x000000ffU;
		green_mask = 0x0000ff00U;
		blue_mask  = 0x00ff0000U;
		pixel_octets = 4;
		break;
	    case PixelBlueGreenRedReserved8BitPerColor:
		red_mask   = 0x00ff0000U;
		green_mask = 0x0000ff00U;
		blue_mask  = 0x000000ffU;
		pixel_octets = 4;
		break;
	    default:
		red_mask   = best_info.PixelInformation.RedMask;
		green_mask = best_info.PixelInformation.GreenMask;
		blue_mask  = best_info.PixelInformation.BlueMask;
		all_mask = red_mask | green_mask | blue_mask |
			   best_info.PixelInformation.ReservedMask;
		if (all_mask <= 0xffU)
			pixel_octets = 1;
		else if (all_mask <= 0xffffU)
			pixel_octets = 2;
		else if (all_mask <= 0xffffffU)
			pixel_octets = 3;
		else
			pixel_octets = 4;
	}
	curr_fg = (((uint64_t)red_mask   * 2 / 3) & red_mask) |
		  (((uint64_t)green_mask * 2 / 3) & green_mask) |
		  (((uint64_t)blue_mask  * 2 / 3) & blue_mask);

	/* Hook up our frame buffer console output routine. */
	replace_con_out();
	splash();
}

/* Undo the frame buffer console set up (in case of a loader error). */
void exit_fb_con(void)
{
	if (orig_txop) {
		EFI_SIMPLE_TEXT_OUT_PROTOCOL *txop = orig_txop;
		ST->ConOut = txop;
		gfxop->SetMode(gfxop, orig_mode);
		txop->Reset(txop, TRUE);
	}
}

/*
 * Do formatted output to the frame buffer console.  As a quick hack, reuse
 * the GNU EFI code to do this...
 */
int cwprintf(const char16_t *fmt, ...)
{
	va_list ap;
	int res;
	va_start(ap, fmt);
	res = (int)VPrint(fmt, ap);
	va_end(ap);
	return res;
}

/*
 * Output a string to the frame buffer console.
 *
 * This should only be called _after_ the frame buffer console is
 * initialized!  Before that happens, code should stick to using Output(.)
 * or ST->ConOut->OutputString(, ).
 */
void cputws(const char16_t *str)
{
	char16_t ch;
	while ((ch = *str++) != 0)
		putwch(ch);
}
