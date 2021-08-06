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
 * Definitions for dealing with PCI I/O --- especially PCI option ROMs
 * (a.k.a. expansion ROMs).
 *
 * According to the _PCI Firmware Specification_, a single option ROM for a
 * particular type of PCI device may comprise one or more ROM _images_, each
 * of which implements the code to drive the device for a different type of
 * system.
 *
 * This header file needs to work in both 32-bit & 64-bit compilation modes.
 */

#ifndef H_PCI
#define H_PCI

#include <inttypes.h>
#include "common.h"

/* Option ROM image header. */
typedef struct __attribute__((packed)) {
	uint16_t sig;			/* 0x55 0xaa signature */
	uint8_t reserved[0x16];		/* reserved */
	uint16_t pcir_off;		/* pointer to PCI Data Structure */
} rimg_hdr_t;

/* PCI Data Structure. */
typedef struct __attribute__((packed)) {
	uint32_t sig;			/* "PCIR" signature */
	uint32_t pci_id;		/* vendor & device id. */
	uint16_t dev_ids_off;		/* ptr. to list of more device ids. */
	uint16_t pcir_sz;		/* size of this structure */
	uint8_t pcir_rev;		/* PCI spec. revision level */
	uint8_t class_if[3];		/* class code */
	uint16_t rimg_sz_hkib;		/* ROM image len. (0x200-byte units) */
	uint16_t vendor_rev;		/* vendor revision level */
	uint8_t type;			/* code type */
	uint8_t flags;			/* last image indicator */
	uint16_t max_rt_sz_hkib;	/* max. image len. at run time (0x200-
					   -byte units) */
	/* ... */
} rimg_pcir_t;

/* rimg_pcir_t::sig value. */
#define PCIR_SIG_PCIR		MAGIC32('P', 'C', 'I', 'R')
/* rimg_pcir_t::type values. */
#define PCIR_TYP_PCAT		0x00	/* PC-AT compatible option ROM */
#define PCIR_TYP_EFI		0x03	/* UEFI option ROM */
/* rimg_pcir_t::flags bit field. */
#define PCIR_FLAGS_LAST_IMAGE	0x80U
/* Minimum size of a PCI Data Structure. */
#define PCIR_MIN_SZ		offsetof(rimg_pcir_t, max_rt_sz_hkib)

/* Extract the PCI vendor id. portion of a unique PCI id. */
static inline uint16_t pci_id_vendor(uint32_t pci_id)
{
	return (uint16_t)pci_id;
}

/* Extract the PCI device id. portion of a unique PCI id. */
static inline uint16_t pci_id_dev(uint32_t pci_id)
{
	return (uint16_t)(pci_id >> 16);
}

/* Combine a vendor id. & a device id. into a complete unique PCI id. */
static inline uint32_t pci_make_id(uint16_t vendor, uint16_t dev)
{
	return (uint32_t)dev << 16 | vendor;
}

#endif
