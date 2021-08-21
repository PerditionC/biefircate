/*
 * Copyright (c) 2021 TK Chia
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

#include <string.h>
#include "stage1/stage1.h"

int memcmp(const void *s1, const void *s2, size_t n)
{
	const unsigned char *p1 = s1, *p2 = s2;
	while (n-- != 0) {
		unsigned char c1 = *p1++, c2 = *p2++;
		if (c1 < c2)
			return -1;
		else if (c1 > c2)
			return +1;
	}
	return 0;
}

void *memmove(void *dest, const void *src, size_t n)
{
	unsigned char *d = dest;
	const unsigned char *s = src;
	if (d < s) {
		while (n-- != 0)
			*d++ = *s++;
	} else {
		d += n;
		s += n;
		while (n-- != 0)
			*--d = *--s;
	}
	return dest;
}

__attribute__((noreturn)) static void wait_and_exit(void)
{
	Output(u"press a key to exit\r\n");
	WaitForSingleEvent(ST->ConIn->WaitForKey, 0);
	Exit(0, 0, NULL);
	for (;;);
}

__attribute__((noreturn)) void
error_with_status(IN CONST CHAR16 *msg, EFI_STATUS status)
{
	Print(u"%Eerror: %s: %d%N\r\n", msg, (INT32)status);
	wait_and_exit();
}

__attribute__((noreturn)) void error(IN CONST CHAR16 *msg)
{
	Print(u"%Eerror: %s%N\r\n", msg);
	wait_and_exit();
}

void warn(IN CONST CHAR16 *msg)
{
	Print(u"%Hwarning: %s%N\r\n", msg);
}

void print_guid(const EFI_GUID *p_guid)
{
	Print(u"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
	    p_guid->Data1, (UINT32)p_guid->Data2, (UINT32)p_guid->Data3,
	    (UINT32)p_guid->Data4[0], (UINT32)p_guid->Data4[1],
	    (UINT32)p_guid->Data4[2], (UINT32)p_guid->Data4[3],
	    (UINT32)p_guid->Data4[4], (UINT32)p_guid->Data4[5],
	    (UINT32)p_guid->Data4[6], (UINT32)p_guid->Data4[7]);
}

EFI_MEMORY_DESCRIPTOR *get_mem_map(UINTN *p_num_ents, UINTN *p_map_key,
    UINTN *p_desc_sz)
{
	UINT32 desc_ver;  /* discarded */
	EFI_MEMORY_DESCRIPTOR *descs = LibMemoryMap(p_num_ents, p_map_key,
	    p_desc_sz, &desc_ver);
	if (!descs || !*p_num_ents)
		error(u"cannot get mem. map!");
	return descs;
}

uint8_t compute_cksum(const void *buf, size_t n)
{
	const uint8_t *p = (const uint8_t *)buf;
	uint8_t cksum = 0;
	while (n-- != 0)
		cksum -= *p++;
	return cksum;
}

void update_cksum(uint8_t *buf, size_t n, uint8_t *p_cksum)
{
	uint8_t cksum = 0;
	*p_cksum = 0;  /* the checksum may be part of the summed area */
	cksum = compute_cksum(buf, n);
	*p_cksum = cksum;
}
