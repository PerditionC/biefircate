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
#include <stdbool.h>
#include "acpi.h"
#include "bparm.h"
#include "common.h"
#include "elf.h"
#include "pci.h"

/* acpi.c functions. */

extern void acpi_init(acpi_xsdp_t *);

/* bmem.c functions. */

extern void bmem_init(void);
extern void *bmem_alloc(UINTN, UINTN);
extern void *bmem_alloc_boottime(UINTN, UINTN);
extern void bmem_fini(EFI_MEMORY_DESCRIPTOR *, UINTN, UINTN,
    uint32_t *, uint32_t *);

/* bparm.c functions. */

extern void *bparm_add(uint32_t, uint32_t);
extern bdat_mem_range_t *bparm_add_mem_range(uint64_t, uint64_t,
    uint32_t, uint32_t, uint64_t);
extern bparm_t *bparm_get(void);

/* fv.c functions. */

extern void fv_init(void);
extern bool fv_find_rimg(uint32_t, uint32_t, void **, uint32_t *);
extern void fv_fini(void);

/* util.c functions. */

extern __attribute__((noreturn)) void error_with_status(IN CONST CHAR16 *,
							EFI_STATUS);
extern __attribute__((noreturn)) void error(IN CONST CHAR16 *);
extern void warn(IN CONST CHAR16 *);
extern void print_guid(const EFI_GUID *);
extern EFI_MEMORY_DESCRIPTOR *get_mem_map(UINTN *, UINTN *, UINTN *);
extern uint8_t compute_cksum(const void *, size_t);
extern void update_cksum(uint8_t *, size_t, uint8_t *);

/* pci.h functions. */

extern const rimg_pcir_t *rimg_find_pcir(const void *, uint64_t);
const uint16_t *rimg_pcir_find_dev_id_list(const rimg_pcir_t *, const void *);
extern void process_pci(void);

/* run-stage2.asm functions. */

extern void run_stage2(Elf32_Addr entry, Elf32_Addr trampoline,
		       unsigned base_kib, uint16_t temp_ebda_seg,
		       bparm_t *bparm);

/* Macros, inline functions, & other definitions. */

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
 * Convert a paragraph-aligned real mode linear address to a real mode
 * segment value.
 */
static inline uint16_t addr_to_rm_seg(uintptr_t p)
{
	return (uint16_t)(p / PARA_SIZE);
}

/* Ditto. */
static inline uint16_t ptr_to_rm_seg(const void *p)
{
	return addr_to_rm_seg((uintptr_t)p);
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

#endif
