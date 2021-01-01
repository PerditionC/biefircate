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

/* Implementation of mbtowc(...) which converts UTF-8 to UTF-16. */

#include <stdlib.h>

int __cdecl mbtowc(wchar_t * __restrict pwc, const char * __restrict s,
		   size_t n)
{
	size_t n_consumed = 0;
	wchar_t st = 0, scratch;
	if (!s) {
		/*
		 * The UTF-8 encoding is stateful at the byte level, but at
		 * the wide character level, it is stateless --- each UTF-8
		 * sequence that results in a wide character is decoded in
		 * the same way.
		 */
		return 0;
	}
	if (!pwc)
		pwc = &scratch;
	while (n-- != 0) {
		unsigned char c = *s++;
		if (!c) {
			if (st)
				return -1;
			*pwc = 0;
			return 0;
		}
		++n_consumed;
		switch (st) {
		    case 0x0020 ... 0x003f:
			/*
			 * 1xxxx --- we already got first byte of 2-byte
			 * UTF-8 sequence.
			 */
			if (c >= 0x80 && c <= 0xbf) {
				st = st << 6 | (c & 0x3f);
				*pwc = st & 0x7ff;
				return n_consumed;
			} else
				return -1;
		    case 0x0010 ... 0x001f:
			/*
			 * 1xxxx --- we got first byte of 3-byte UTF-8
			 * sequence.
			 */
			if (c >= 0x80 && c <= 0xbf) {
				st = st << 6 | (c & 0x3f);
				break;
			} else
				return -1;
		    case 0x0400 ... 0x07ff:
			/*
			 * 1xxxx xxxxxx --- we got first 2 bytes of 3-byte
			 * UTF-8 sequence.
			 */
			if (c >= 0x80 && c <= 0xbf) {
				st = st << 6 | (c & 0x3f);
				switch (st & 0xffff) {
				    case 0x0000 ... 0x07ff:
				    case 0xd800 ... 0xdfff:
				    case 0xfffe ... 0xffff:
					return -1;
				    default:
					*pwc = st & 0xffff;
					return n_consumed;
				}
			}
		    default:
			/* Not in the middle of a UTF-8 sequence. */
			switch (c) {
			    case 0x00 ... 0x7f:  /* US-ASCII */
				*pwc = c;
				return n_consumed;
			    case 0xc2 ... 0xdf:  /* start of 2-byte UTF-8 */
				st = (c ^ 0xc0) ^ 0x20;
				break;
			    case 0xe0 ... 0xef:  /* start of 3-byte UTF-8 */
				st = (c ^ 0xe0) ^ 0x10;
				break;
			    default:
				return -1;
			}
		}
	}
	return -1;
}
