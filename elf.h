/*
 * 32-bit Executable and Linking Format (ELF) structures.  See:
 *
 *	TIS Committee.  _Tool Interface Standard (TIS) Executable and
 *	Linking Format (ELF) Specification: Version 1.2_.  May 1995.
 *	http://refspecs.linuxbase.org/elf/elf.pdf .
 */

#ifndef H_ELF
#define H_ELF

#include <inttypes.h>

typedef uint16_t Elf32_Half;
typedef uint32_t Elf32_Word, Elf32_Addr, Elf32_Off;

#define EI_NIDENT	16

/* Overall ELF header. */
typedef struct __attribute__((packed)) {
	uint8_t e_ident[EI_NIDENT];
	Elf32_Half e_type, e_machine;
	Elf32_Word e_version;
	Elf32_Addr e_entry;
	Elf32_Off e_phoff, e_shoff;
	Elf32_Word e_flags;
	Elf32_Half e_ehsize, e_phentsize, e_phnum,
		   e_shentsize, e_shnum, e_shstrndx;
} Elf32_Ehdr;

/* e_ident[] field indices. */
#define EI_MAG0		0
#define EI_MAG1		1
#define EI_MAG2		2
#define EI_MAG3		3
#define EI_CLASS	4
#define EI_DATA		5
#define EI_VERSION	6
#define EI_PAD		7

/* e_ident[] field values. */
#define ELFMAG0		0x7f
#define ELFMAG1		0x45
#define ELFMAG2		0x4c
#define ELFMAG3		0x46
#define ELFCLASS32	1
#define ELFDATA2LSB	1
#define EV_CURRENT	1
#define ET_EXEC		2
#define EM_386		3

/* Program header. */
typedef struct __attribute__((packed)) {
	Elf32_Word p_type;
	Elf32_Off p_offset;
	Elf32_Addr p_vaddr, p_paddr;
	Elf32_Word p_filesz, p_memsz, p_flags, p_align;
} Elf32_Phdr;

/* p_type field values. */
#define PT_LOAD		1

#endif
