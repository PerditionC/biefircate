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

#ifndef H_COMMON
#define H_COMMON

#include <inttypes.h>

#define PARA_SIZE	0x10UL		/* no. of bytes in a paragraph */
#define KIBYTE		1024UL		/* no. of bytes in a KiB */
#define HKIBYTE		(KIBYTE / 2)	/* no. of bytes in half a KiB */
#define BMEM_MAX_ADDR	0x100000ULL	/* end of base memory, i.e. the
					   1 MiB mark */

/* Fabricate a 32-bit magic number from 4 characters. */
#define MAGIC32(a, b, c, d) \
	((uint32_t)(unsigned char)(a)	    | \
	 (uint32_t)(unsigned char)(b) <<  8 | \
	 (uint32_t)(unsigned char)(c) << 16 | \
	 (uint32_t)(unsigned char)(d) << 24)

/* Fabricate a 64-bit magic number from 8 characters. */
#define MAGIC64(a, b, c, d, e, f, g, h) \
	((uint64_t)(unsigned char)(a)	    | \
	 (uint64_t)(unsigned char)(b) <<  8 | \
	 (uint64_t)(unsigned char)(c) << 16 | \
	 (uint64_t)(unsigned char)(d) << 24 | \
	 (uint64_t)(unsigned char)(e) << 32 | \
	 (uint64_t)(unsigned char)(f) << 40 | \
	 (uint64_t)(unsigned char)(g) << 48 | \
	 (uint64_t)(unsigned char)(h) << 56)

/*
 * Address range types, in the manner of BIOS int 0x15, ax = 0xe820.  These
 * go into bdat_mem_range_t::e820_type.  Other than E820_DISABLED, the names
 * are taken from the Linux 5.9.14 kernel code.
 */
#define E820_RAM	1U		/* available memory */
#define E820_RESERVED	2U		/* reserved memory */
#define E820_ACPI	3U		/* ACPI reclaimable */
#define E820_NVS	4U		/* ACPI NVS */
#define E820_UNUSABLE	5U		/* bad memory */
#define E820_DISABLED	6U		/* disabled memory (ACPI 6.3) */
#define E820_PMEM	7U		/* persistent memory (ACPI 6.3) */

/* Wait for an asynchronous interrupt. */
static inline void hlt(void)
{
	__asm volatile("hlt" : : : "memory");
}

/* Type of a 64-bit pointer. */
#ifndef __x86_64__
typedef uint64_t ptr64_t;
#else
typedef void *ptr64_t;
#endif

#endif
