/*
 * Copyright (c) 2021 TK Chia
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

/*
 * cprintf(...) function for formatted frame buffer console output, built on
 * top of Charles Nicholson's nanoprintf.
 */

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "truckload.h"

#define NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS	1
#define NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS	1
#define NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS		0
#define NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS		1
#define NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS	0
#define NANOPRINTF_VISIBILITY_STATIC
#define NANOPRINTF_IMPLEMENTATION

#include "nanoprintf/nanoprintf.h"

typedef struct {
	size_t tail;
	char buf[MB_LEN_MAX];
} printf_ctx_t;

static void cprintf_putc(int c, void *p)
{
	printf_ctx_t *ctx = (printf_ctx_t *)p;
	size_t head = 0, tail = ctx->tail;
	int consumed;
	wchar_t wc;
	if (!c)
		return;
	if (tail == MB_LEN_MAX) {
		putwch(0xfffd);
		memmove(ctx->buf, ctx->buf + 1, MB_LEN_MAX - 1);
		ctx->buf[MB_LEN_MAX - 1] = c;
	} else {
		ctx->buf[tail] = c;
		++tail;
	}
	while ((consumed = mbtowc(&wc, ctx->buf + head, tail - head)) > 0) {
		putwch(wc);
		head += consumed;
	}
	if (head != 0 && head != tail)
		memmove(ctx->buf, ctx->buf + head, tail - head);
	ctx->tail = tail - head;
}

static void cprintf_flush(printf_ctx_t *ctx)
{
	if (ctx->tail)
		cnputs(ctx->buf, ctx->tail);
}

void vcprintf(const char *fmt, va_list ap)
{
	printf_ctx_t ctx = { 0, };
	npf_vpprintf(cprintf_putc, &ctx, fmt, ap);
	cprintf_flush(&ctx);
}

void cprintf(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vcprintf(fmt, ap);
	va_end(ap);
}
