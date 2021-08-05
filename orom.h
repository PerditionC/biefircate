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
 * Definitions for dealing with PC-AT compatible PCI option ROMs (a.k.a.
 * expansion ROMs).
 *
 * This header file needs to work in both 32-bit & 64-bit compilation modes.
 */

#ifndef H_OROM
#define H_OROM

#include <inttypes.h>
#include "common.h"

/* Option ROM header. */
typedef struct __attribute__((packed)) {
	uint16_t sig;			/* 0x55 0xaa signature */
	uint8_t reserved[0x16];		/* reserved */
	uint16_t pcir_off;		/* pointer to PCI Data Structure */
} orom_hdr_t;

/* PCI Data Structure. */
typedef struct __attribute__((packed)) {
	uint32_t sig;			/* "PCIR" signature */
	uint32_t pci_id;		/* vendor & device id. */
	uint16_t dev_ids_off;		/* ptr. to list of more device ids. */
	uint16_t pcir_sz;		/* size of this structure */
	uint8_t pcir_rev;		/* PCI spec. revision level */
	uint8_t class_if[3];		/* class code */
	uint16_t orom_sz_hkib;		/* ROM image len. (0x200-byte units) */
	uint16_t vendor_rev;		/* vendor revision level */
	uint8_t type;			/* code type */
	uint8_t flags;			/* last image indicator */
	uint16_t max_rt_sz_hkib;	/* max. image len. at run time (0x200-
					   -byte units) */
	/* ... */
} orom_pcir_t;

/* orom_pcir_t::sig value. */
#define PCIR_SIG_PCIR		MAGIC32('P', 'C', 'I', 'R')
/* orom_pcir_t::type values. */
#define PCIR_TYP_PCAT		0x00	/* PC-AT compatible option ROM */
#define PCIR_TYP_EFI		0x03	/* UEFI option ROM */
/* orom_pcir_t::flags bit field. */
#define PCIR_FLAGS_LAST_IMAGE	0x80U
/* Minimum size of a PCI Data Structure. */
#define PCIR_MIN_SZ		offsetof(orom_pcir_t, max_rt_sz_hkib)

#endif
