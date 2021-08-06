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

#include <stdbool.h>
#include <string.h>
#include "fv-proto.h"

static EFI_GUID gEfiFirmwareVolume2ProtocolGuid =
		    { 0x220e73b6, 0x6bdb, 0x4413,
		      { 0x84, 0x05, 0xb9, 0x74, 0xb1, 0x08, 0x61, 0x9a } };

#define MAX_OROM_SZ	0xf0000ULL
#define HASH_BUCKETS	1381

typedef struct ht_node {
	struct ht_node *next;
	uint32_t pci_id, class_if;
	void *rimg_copy;
	uint32_t rimg_sz;
} ht_node_t;

static ht_node_t *ht[HASH_BUCKETS];

static unsigned fv_hash_bucket(uint32_t pci_id, uint32_t class_if)
{
	return (unsigned)(((uint64_t)pci_id << 32 | class_if) % HASH_BUCKETS);
}

static void fv_add_hash_entry(uint32_t pci_id, uint32_t class_if,
    void *rimg_copy, uint32_t sz)
{
	unsigned bucket = fv_hash_bucket(pci_id, class_if);
	ht_node_t *node = AllocatePool(sizeof(ht_node_t));
	if (!node)
		error(u"no mem. to cache option ROM image!");
	node->next = ht[bucket];
	node->pci_id = pci_id;
	node->class_if = class_if;
	node->rimg_copy = rimg_copy;
	node->rimg_sz = sz;
	ht[bucket] = node;
}

static void fv_cache_rimg(const void *rimg, uint32_t sz,
    const rimg_pcir_t *pcir, const void *rom_end)
{
	uint32_t class_if = (uint32_t)pcir->class_if[2] << 24 |
			    (uint32_t)pcir->class_if[1] << 16 |
			    (uint32_t)pcir->class_if[0] <<  8;
	uint32_t pci_id_0 = pcir->pci_id, pci_id;
	uint16_t vendor = pci_id_vendor(pci_id_0);
	const uint16_t *dev_ids;
	uint16_t dev;
	void *rimg_copy = AllocatePool(sz);
	if (!rimg_copy)
		error(u"no mem. to cache ROM img.!");
	memcpy(rimg_copy, rimg, sz);
	dev_ids = rimg_pcir_find_dev_id_list(pcir, rom_end);
	Print(u"      cache ROM img. @0x%lx~@0x%lx for %04x:%04x%s "
		     "%02x %02x %02x\r\n",
	    rimg_copy, (char *)rimg_copy + sz - 1,
	    (UINT32)vendor, (UINT32)pci_id_dev(pci_id_0),
	    dev_ids ? u" etc." : u"",
	    class_if >> 24, (class_if >> 16) & 0xffU, (class_if >> 8) & 0xffU);
	fv_add_hash_entry(pci_id_0, class_if, rimg_copy, sz);
	if (dev_ids) {
		while ((dev = *dev_ids++) != 0) {
			pci_id = pci_make_id(vendor, dev);
			if (pci_id != pci_id_0)
				fv_add_hash_entry(pci_id, class_if,
				    rimg_copy, sz);
		}
	}
}

static void fv_gather_rimgs_for_one_sxn(const void *rom, UINTN rom_sz,
    const EFI_GUID *p_guid, UINTN instance)
{
	const void *rom_left = rom;
	const void *rom_end = (const uint8_t *)rom + rom_sz;
	uint64_t rom_left_sz = rom_sz, found_sz = 0;
	uint32_t this_sz;
	const rimg_pcir_t *found_pcir = NULL, *pcir;
	const void *found_rimg = NULL;
	bool saw_a_pcir = false;
	while ((pcir = rimg_find_pcir(rom_left, rom_left_sz)) != NULL) {
		if (!saw_a_pcir) {
			Output(u"    ");
			print_guid(p_guid);
			Print(u" raw sec. %lx is option ROM\r\n", instance);
			saw_a_pcir = true;
		}
		this_sz = (uint32_t)pcir->rimg_sz_hkib * HKIBYTE;
		if (pcir->type == PCIR_TYP_PCAT) {
			if (!found_rimg ||
			    (found_pcir->pcir_rev < 3 && pcir->pcir_rev >= 3))
			{
				found_rimg = rom_left;
				found_sz = this_sz;
				found_pcir = pcir;
			}
		}
		if ((pcir->flags & PCIR_FLAGS_LAST_IMAGE) != 0) {
			if (found_rimg) {
				fv_cache_rimg(found_rimg, found_sz,
					      found_pcir, rom_end);
				found_rimg = NULL;
			}
		}
		rom_left = (const char *)rom_left + this_sz;
		rom_left_sz -= this_sz;
	}
}

static void fv_gather_rimgs_for_one_file(EFI_FIRMWARE_VOLUME2_PROTOCOL *fv,
    EFI_GUID *p_guid)
{
	UINTN instance = 0;
	void *sxn = AllocatePool(MAX_OROM_SZ);
	do {
		UINTN sxn_sz = MAX_OROM_SZ;
		UINT32 auth;
		EFI_STATUS status = fv->ReadSection(fv, p_guid,
		    EFI_SECTION_RAW, instance, &sxn, &sxn_sz, &auth);
		if (EFI_ERROR(status))
			break;
		if (sxn_sz > MAX_OROM_SZ)
			sxn_sz = MAX_OROM_SZ;
		fv_gather_rimgs_for_one_sxn(sxn, sxn_sz, p_guid, instance);
	} while (++instance != 0);
	FreePool(sxn);
}

static void fv_gather_rimgs_for_one_fv(EFI_FIRMWARE_VOLUME2_PROTOCOL *fv)
{
	EFI_STATUS status;
	EFI_FV_FILETYPE type;
	EFI_GUID guid;
	EFI_FV_FILE_ATTRIBUTES attrs;
	UINTN sz;
	void *key = AllocateZeroPool(fv->KeySize);
	if (!key)
		error(u"not enough mem. for FV key");
	for (;;) {
		type = EFI_FV_FILETYPE_ALL;
		status = fv->GetNextFile(fv, key, &type, &guid, &attrs, &sz);
		if (EFI_ERROR(status))
			break;
		switch (type) {
		    case EFI_FV_FILETYPE_FFS_PAD:
		    case EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE:
			continue;
		    default:
			;
		}
		fv_gather_rimgs_for_one_file(fv, &guid);
	}
	FreePool(key);
}

void fv_init(void)
{
	EFI_HANDLE *handles;
	UINTN num_handles, hidx;
	unsigned bucket;
	EFI_STATUS status = LibLocateHandle(ByProtocol,
	    &gEfiFirmwareVolume2ProtocolGuid, NULL, &num_handles, &handles);
	if (EFI_ERROR(status) || !num_handles) {
		Output(u"no EFI firmware volumes avail.?\r\n");
		return;
	}
	Print(u"EFI firmware volumes: %lu\r\n", num_handles);
	for (bucket = 0; bucket < HASH_BUCKETS; ++bucket)
		ht[bucket] = NULL;
	for (hidx = 0; hidx < num_handles; ++hidx) {
		EFI_FIRMWARE_VOLUME2_PROTOCOL *fv;
		EFI_HANDLE handle = handles[hidx];
		EFI_STATUS status = BS->HandleProtocol(handle,
		    &gEfiFirmwareVolume2ProtocolGuid, (void **)&fv);
		if (EFI_ERROR(status))
			continue;
		Print(u"  scanning FV %lu\r\n", hidx);
		fv_gather_rimgs_for_one_fv(fv);
	}
	FreePool(handles);
}

bool fv_find_rimg(uint32_t pci_id, uint32_t class_if,
    void **p_rimg_copy, uint32_t *p_sz)
{
	unsigned bucket = fv_hash_bucket(pci_id, class_if);
	ht_node_t *node = ht[bucket];
	while (node) {
		if (node->pci_id == pci_id && node->class_if == class_if) {
			*p_rimg_copy = node->rimg_copy;
			*p_sz = node->rimg_sz;
			return true;
		}
		node = node->next;
	}
	return false;
}

void fv_fini(void)
{
}
