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
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "efi-stuff.h"
#include "truckload.h"
#include "font-default.h"

#if FONT_DEFAULT_WIDTH != 8
#   error "unsupported font width"
#endif

#define VID_MIN_TEXT_WIDTH	100
#define VID_MIN_TEXT_HEIGHT	40
#define VID_MIN_PIX_WIDTH	(VID_MIN_TEXT_WIDTH * FONT_DEFAULT_WIDTH)
#define VID_MIN_PIX_HEIGHT	(VID_MIN_TEXT_HEIGHT * FONT_DEFAULT_HEIGHT)

extern EFI_GUID gEfiGraphicsOutputProtocolGuid;
extern void *_memset_pat2(volatile void *, uint16_t, size_t),
	    *_memset_pat3(volatile void *, uint32_t, size_t),
	    *_memset_pat4(volatile void *, uint32_t, size_t);

typedef struct __attribute__((packed)) {
	uint8_t a8_0, a8_1, a8_2;
} v3qi_t;

typedef volatile union {
	uint8_t a8[1];
	uint16_t a16[1];
	v3qi_t a24[1];
	uint32_t a32[1];
} fb_t;

static const struct { uint8_t b, g, r; } colour_mapping[] = {
	[BLACK]		= { 0x00, 0x00, 0x00 },
	[BLUE]		= { 0xaa, 0x00, 0x00 },
	[GREEN]		= { 0x00, 0xaa, 0x00 },
	[CYAN]		= { 0xaa, 0xaa, 0x00 },
	[RED]		= { 0x00, 0x00, 0xaa },
	[MAGENTA]	= { 0xaa, 0x00, 0xaa },
	[BROWN]		= { 0x00, 0x55, 0xaa },
	[LIGHTGRAY]	= { 0xaa, 0xaa, 0xaa },
	[DARKGRAY]	= { 0x55, 0x55, 0x55 },
	[LIGHTBLUE]	= { 0xff, 0x55, 0x55 },
	[LIGHTGREEN]	= { 0x55, 0xff, 0x55 },
	[LIGHTCYAN]	= { 0xff, 0xff, 0x55 },
	[LIGHTRED]	= { 0x55, 0x55, 0xff },
	[LIGHTMAGENTA]	= { 0xff, 0x55, 0xff },
	[YELLOW]	= { 0x55, 0xff, 0xff },
	[WHITE]		= { 0xff, 0xff, 0xff },
};
static EFI_GRAPHICS_OUTPUT_PROTOCOL *gfxop;
static UINT32 orig_mode;
static UINT32 text_width, text_height, pix_width, pix_vis_width, pix_height,
	      red_mask, green_mask, blue_mask, curr_bg = 0, curr_fg = 0,
	      pixel_octets = 0;
static fb_t *frame_buf;
static unsigned curr_row = 0, curr_col = 0;

static void splash(bool use_fb_p)
{
	if (use_fb_p)
		cputws(u".:. " PACKAGE_NAME " " PACKAGE_VERSION " .:.\r\n");
	else
		Output(u".:. " PACKAGE_NAME " " PACKAGE_VERSION " .:.\r\n");
}

/* Messy hack to suppress some GCC type warnings. */
#define APrint(fmt, ...) \
	APrint((const unsigned char *)(fmt), __VA_ARGS__)

static void dump_mode_info(UINT32 mode_num,
			   const EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info)
{
	APrint("mode 0x%04x  %4u", mode_num, info->HorizontalResolution);
	if (info->PixelsPerScanLine != info->HorizontalResolution)
		Print(u"{%4u}", info->PixelsPerScanLine);
	APrint("*%4u  ", info->VerticalResolution);
	switch (info->PixelFormat) {
	    case PixelRedGreenBlueReserved8BitPerColor:
		Output(u"RGBX8888");  break;
	    case PixelBlueGreenRedReserved8BitPerColor:
		Output(u"BGRX8888");  break;
	    case PixelBitMask:
		APrint("RGBX:%x/%x/%x/%x",
		       info->PixelInformation.RedMask,
		       info->PixelInformation.GreenMask,
		       info->PixelInformation.BlueMask,
		       info->PixelInformation.ReservedMask);
		break;
	    default:
		Output(u"BLT-only");
	}
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

static void fb_con_move_rows(UINT32 src_start_row, UINT32 n_rows,
			     UINT32 dest_start_row)
{
	size_t row_octets = (size_t)pix_width * FONT_DEFAULT_HEIGHT
			    * pixel_octets;
	memmove((void *)&frame_buf->a8[dest_start_row * row_octets],
		(const void *)&frame_buf->a8[src_start_row * row_octets],
		n_rows * row_octets);
}

static void fb_con_blank_rows(UINT32 start_row, UINT32 n_rows)
{
	size_t row_pixels = (size_t)pix_width * FONT_DEFAULT_HEIGHT;
	switch (pixel_octets) {
	    case 1:
		memset((void *)&frame_buf->a8[start_row * row_pixels],
		       (uint8_t)curr_bg, n_rows * row_pixels);
		break;
	    case 2:
		_memset_pat2(&frame_buf->a16[start_row * row_pixels],
			     (uint16_t)curr_bg, n_rows * row_pixels * 2);
		break;
	    case 3:
		_memset_pat3(&frame_buf->a24[start_row * row_pixels],
			     (uint32_t)curr_bg, n_rows * row_pixels * 3);
		break;
	    default:
		_memset_pat4(&frame_buf->a32[start_row * row_pixels],
			     curr_bg, n_rows * row_pixels * 4);
	}
}

static void fb_con_nl(void)
{
	curr_col = 0;
	if (curr_row < text_height - 1) {
		++curr_row;
		return;
	}
	fb_con_move_rows(1, text_height - 1, 0);
	fb_con_blank_rows(text_height - 1, 1);
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

static void early_panic(IN CONST CHAR16 *msg, EFI_STATUS status)
{
	APrint("\r\npanic: %s: 0x%lx", msg, (UINT16)status);
	freeze();
}

/* Initialize the frame buffer console. */
INIT_TEXT void fb_con_init(void)
{
	UINT32 mode_num, max_mode_num, best_mode_num, avail_mode_count = 0;
	EFI_GRAPHICS_OUTPUT_MODE_INFORMATION best_info = { };
	EFI_STATUS status;
	UINT32 all_mask;

	/* Find a good graphics mode to switch to. */
	splash(false);
	status = BS->LocateProtocol(&gEfiGraphicsOutputProtocolGuid,
	    NULL, (void **)&gfxop);
	if (EFI_ERROR(status))
		early_panic(u"cannot get EFI_GRAPHICS_OUTPUT_PROTOCOL",
		    status);
	orig_mode = gfxop->Mode->Mode;
	APrint("current UEFI graphics mode: 0x%x\r\n"
	       "available framebuffer modes:", orig_mode);
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
		if (avail_mode_count % 2 == 0)
			Output(u"\r\n");
		++avail_mode_count;
		Output(u" | ");
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
	Output(u"\r\n");
	if (best_mode_num == max_mode_num)
		early_panic(u"no suitable graphics mode to switch to!", 0);

	/* Switch to the new graphics mode. */
	APrint("switching to mode 0x%x in 3", best_mode_num);
	status = WaitForSingleEvent(ST->ConIn->WaitForKey, 10000000ULL);
	if (EFI_ERROR(status)) {
		Output(u" 2");
		status = WaitForSingleEvent(ST->ConIn->WaitForKey,
					    10000000ULL);
		if (EFI_ERROR(status)) {
			Output(u" 1");
			WaitForSingleEvent(ST->ConIn->WaitForKey,
					   10000000ULL);
		}
	}
	status = gfxop->SetMode(gfxop, best_mode_num);
	if (EFI_ERROR(status))
		early_panic(u"cannot set graphics mode", status);

	/* Set things up for frame buffer console output. */
	pix_width = best_info.PixelsPerScanLine;
	pix_vis_width = best_info.HorizontalResolution;
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
	textcolor(LIGHTGRAY);

	/* Start using our own console output functions to spew some stuff. */
	splash(true);
	cprintf("now using frame buffer console @%p, "
		"text %" PRIu32 "*%" PRIu32 ", pixels %" PRIu32,
	    frame_buf, text_width, text_height, pix_width);
	if (pix_vis_width != pix_width)
		cprintf("{%" PRIu32 "}", pix_vis_width);
	cprintf("*%" PRIu32 ", %" PRIu32 " octet%s/pixel\n",
	    pix_height, pixel_octets, pixel_octets == 1 ? "" : "s");
}

/* Return the address after the end of the frame buffer console memory. */
INIT_TEXT uintptr_t fb_con_mem_end(void)
{
	return (uintptr_t)&frame_buf->
	    a8[(size_t)pix_width * (size_t)pix_height * (size_t)pixel_octets];
}

/*
 * Output a character to the frame buffer console.
 *
 * This should only be called _after_ the frame buffer console is
 * initialized!  Before that happens, code should stick to using Output(.)
 * or ST->ConOut->OutputString(, ).
 */
void putwch(char16_t ch)
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

void cnputs(const char *str, size_t n)
{
	int nc;
	wchar_t ch;
	while (n != 0) {
		nc = mbtowc(&ch, str, n);
		if (nc <= 0) {
			ch = 0xfffd;
			nc = 1;
		}
		n -= nc;
		str += nc;
		putwch(ch);
	}
}

void cputs(const char *str)
{
	cnputs(str, strlen(str));
}

/* Set the foreground colour for text. */
enum COLORS textcolor(enum COLORS colour)
{
	static enum COLORS save_colour = BLACK;
	enum COLORS old_colour = save_colour;
	save_colour = colour;
	uint8_t red_share   = colour_mapping[colour].r;
	uint8_t green_share = colour_mapping[colour].g;
	uint8_t blue_share  = colour_mapping[colour].b;
	curr_fg = (((uint64_t)red_mask   * red_share   / 0xff) & red_mask) |
		  (((uint64_t)green_mask * green_share / 0xff) & green_mask) |
		  (((uint64_t)blue_mask  * blue_share  / 0xff) & blue_mask);
	return old_colour;
}

/*
 * Set the foreground text colour to a colour for displaying warning
 * messages.
 */
enum COLORS warnvideo(void)
{
	return textcolor(LIGHTMAGENTA);
}
