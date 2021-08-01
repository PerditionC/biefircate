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

/*
 * Data structures for reporting information about a multiprocessor system
 * to a legacy BIOS operating system.
 *
 *	Intel Corporation.  _MultiProcessor Specification: Version 1.4_. 
 *	May 1997.
 */

#ifndef H_MP
#define H_MP

#include <inttypes.h>

/* MP floating pointer structure. */
typedef struct __attribute__((packed)) {
	uint32_t sig;			/* "_MP_" signature */
	uint32_t mp_conf_addr;		/* addr. of MP conf. table */
	uint8_t len;			/* length of this table */
	uint8_t spec_rev;		/* MP spec. ver. no. */
	uint8_t cksum;			/* checksum */
	uint8_t features[5];		/* MP feature info. bytes */
} intel_mp_flt_t;

#define MP_FEAT1_IMCRP	0x80

/* MP configuration table header. */
typedef struct __attribute__((packed)) {
	uint32_t sig;			/* "PCMP" signature */
	uint16_t base_tbl_len;		/* length of base conf. table */
	uint8_t spec_rev;		/* MP spec. ver. no. */
	uint8_t cksum;			/* checksum */
	char oem_id[8];			/* OEM id. */
	char prod_id[12];		/* product id. */
	uint32_t oem_tbl_ptr;		/* addr. of OEM-defined conf. table */
	uint16_t oem_tbl_sz;		/* OEM table size */
	uint16_t ent_cnt;		/* # entries in base table */
	uint32_t lapic_addr;		/* addr. of local APIC */
	uint16_t ext_tbl_len;		/* length of ext. conf. table */
	uint8_t ext_tbl_cksum;		/* checksum for ext. conf. table */
	uint8_t reserved;		/* reserved */
} intel_mp_conf_hdr_t;

/* MP configuration table entry for a processor. */
typedef struct __attribute__((packed)) {
	uint8_t type;			/* entry type (= MP_PROC) */
	uint8_t lapic_id;		/* local APIC id. */
	uint8_t lapic_ver;		/* local APIC version */
	uint8_t cpu_flags;		/* flags about CPU */
	uint32_t cpu_sig;		/* CPU signature */
	uint32_t cpu_features;		/* feature flags for CPU */
} intel_mp_cpu_t;

#define MP_CPU		0
#define MP_CPU_EN	0x01
#define MP_CPU_BP	0x02

/* MP configuration table entry for an I/O APIC. */
typedef struct __attribute__((packed)) {
	uint8_t type;			/* entry type (= MP_IOAPIC) */
	uint8_t ic_id;			/* I/O APIC id. */
	uint8_t ic_ver;			/* I/O APIC version */
	uint8_t ic_flags;		/* I/O APIC flags */
	uint32_t ic_addr;		/* I/O APIC base address */
} intel_mp_ioapic_t;

#define MP_IOAPIC	2
#define MP_IOAPIC_EN	0x01
#define IOAPIC_ADDR	0xfec00000ULL	/* assumed address of the I/O APIC */

#endif
