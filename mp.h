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
	char sig[4];			/* "_MP_" signature */
	uint32_t mp_conf_addr;		/* addr. of MP conf. table */
	uint8_t len;			/* length of this table */
	uint8_t spec_rev;		/* MP spec. ver. no. */
	uint8_t cksum;			/* checksum */
	uint8_t features[5];		/* MP feature info. bytes */
} intel_mp_flt_t;

#define MP_FEAT1_IMCRP	0x80

/* MP configuration table header. */
typedef struct __attribute__((packed)) {
	char sig[4];			/* "PCMP" signature */
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
