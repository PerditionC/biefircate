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

#ifndef H_STAGE1_STAGE1
#define H_STAGE1_STAGE1

#define GNU_EFI_USE_MS_ABI
#include <efi.h>
#include <efilib.h>
#include <inttypes.h>

/* bmem.c functions. */

extern void bmem_init(void);
extern void *bmem_alloc(UINTN, UINTN);
extern void *bmem_alloc_boottime(UINTN, UINTN);
extern unsigned bmem_fini(void);

/* util.c functions. */

extern __attribute__((noreturn)) void error_with_status(IN CONST CHAR16 *,
							EFI_STATUS);
extern __attribute__((noreturn)) void error(IN CONST CHAR16 *);
extern EFI_MEMORY_DESCRIPTOR *get_mem_map(UINTN *, UINTN *, UINTN *);

/* Macros, inline functions, & other definitions. */

#define PARA_SIZE	0x10UL		/* no. of bytes in a paragraph */
#define KIBYTE		1024UL		/* no. of bytes in a KiB */
#define BMEM_MAX_ADDR	0x100000ULL	/* end of base memory, i.e. the
					   1 MiB mark */

/* Define a bit vector type for storing the given number of bits. */
#define BVEC_TYPE(num_ents) \
	struct { UINT32 __bits[((num_ents) + 32ULL - 1) / 32]; }

/* Test whether a bit in a bit vector is set. */
static inline bool bvec_test(const void *bvec, UINT32 idx)
{
	int res;
	__asm volatile("btl %2, %a1; sbbl %0, %0"
	    : "=r" (res) : "p" (bvec), "r" (idx) : "cc");
	return res;
}

/* Set a bit in a bit vector. */
static inline void bvec_set(void *bvec, UINT32 idx)
{
	__asm volatile("btsl %1, %a0"
	    : : "p" (bvec), "r" (idx) : "cc", "memory");
}

/* Clear a bit in a bit vector. */
static inline void bvec_clear(void *bvec, UINT32 idx)
{
	__asm volatile("btrl %1, %a0"
	    : : "p" (bvec), "r" (idx) : "cc", "memory");
}

/*
 * Iterate over a memory map returned by LibMemoryMap or EFI_BOOT_SERVICES
 * .GetMemoryMap(...).
 */
#define FOR_EACH_MEM_DESC(desc, descs, desc_sz, num_ents, ent_iter) \
	for ((ent_iter) = (num_ents), (desc) = (descs); \
	     (ent_iter); \
	     --(ent_iter), \
	     (desc) = (EFI_MEMORY_DESCRIPTOR *)((char *)(desc) + (desc_sz)))

/* Fabricate a 32-bit magic number from 4 characters. */
#define MAGIC32(a, b, c, d) \
	((uint32_t)(unsigned char)(a)	    | \
	 (uint32_t)(unsigned char)(b) <<  8 | \
	 (uint32_t)(unsigned char)(c) << 16 | \
	 (uint32_t)(unsigned char)(d) << 24)

#endif
