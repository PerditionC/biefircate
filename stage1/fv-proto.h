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
 * Definitions for the UEFI Firmware Volume Protocol.  These are derived
 * from <Protocol/FirmwareVolume2.h>, <Pi/PiFirmwareFile.h>, &
 * <Pi/PiFirmwareVolume.h> in Intel's EDK II development environment.
 */

#ifndef H_STAGE1_FV_PROTO
#define H_STAGE1_FV_PROTO

#include "stage1/stage1.h"

typedef UINT8 EFI_FV_FILETYPE, EFI_SECTION_TYPE;
typedef UINT32 EFI_FV_FILE_ATTRIBUTES, EFI_FV_WRITE_POLICY;
typedef UINT64 EFI_FV_ATTRIBUTES;

/* EFI_FV_FILETYPE values. */
#define EFI_FV_FILETYPE_ALL			0x00
#define EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE	0x0b
#define EFI_FV_FILETYPE_FFS_PAD			0xf0

/* EFI_SECTION_TYPE values. */
#define EFI_SECTION_ALL		0x00
#define EFI_SECTION_RAW		0x19

typedef struct EFI_FV_WRITE_FILE_DATA EFI_FV_WRITE_FILE_DATA;

struct EFI_FIRMWARE_VOLUME2_PROTOCOL;

typedef struct EFI_FIRMWARE_VOLUME2_PROTOCOL {
	EFI_STATUS (EFIAPI *GetVolumeAttributes)
	    (IN CONST struct EFI_FIRMWARE_VOLUME2_PROTOCOL *,
	     OUT EFI_FV_ATTRIBUTES *);
	EFI_STATUS (EFIAPI *SetVolumeAttributes)
	    (IN CONST struct EFI_FIRMWARE_VOLUME2_PROTOCOL *,
	     IN OUT EFI_FV_ATTRIBUTES *);
	EFI_STATUS (EFIAPI *ReadFile)
	    (IN CONST struct EFI_FIRMWARE_VOLUME2_PROTOCOL *,
	     IN CONST EFI_GUID *, IN OUT VOID **, IN OUT UINTN *,
	     OUT EFI_FV_FILETYPE *, OUT EFI_FV_FILE_ATTRIBUTES *, OUT UINT32 *);
	EFI_STATUS (EFIAPI *ReadSection)
	    (IN CONST struct EFI_FIRMWARE_VOLUME2_PROTOCOL *,
	     IN CONST EFI_GUID *, IN EFI_SECTION_TYPE, IN UINTN,
	     IN OUT VOID **, IN OUT UINTN *, OUT UINT32 *);
	EFI_STATUS (EFIAPI *WriteFile)
	    (IN CONST struct EFI_FIRMWARE_VOLUME2_PROTOCOL *, IN UINT32,
	     IN EFI_FV_WRITE_POLICY, IN EFI_FV_WRITE_FILE_DATA *);
	EFI_STATUS (EFIAPI *GetNextFile)
	    (IN CONST struct EFI_FIRMWARE_VOLUME2_PROTOCOL *, IN OUT VOID *,
	     IN OUT EFI_FV_FILETYPE *, OUT EFI_GUID *,
	     OUT EFI_FV_FILE_ATTRIBUTES *, OUT UINTN *);
	UINT32 KeySize;
	EFI_HANDLE ParentHandle;
	EFI_STATUS (EFIAPI *GetInfo)
	    (IN CONST struct EFI_FIRMWARE_VOLUME2_PROTOCOL *,
	     IN CONST EFI_GUID *, IN OUT UINTN *, OUT VOID *);
	EFI_STATUS (EFIAPI *SetInfo)
	    (IN CONST struct EFI_FIRMWARE_VOLUME2_PROTOCOL *,
	     IN CONST EFI_GUID *, IN UINTN, IN CONST VOID *);
} EFI_FIRMWARE_VOLUME2_PROTOCOL;

#endif
