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

/* GNU ld compatible linker script for stage 2. */

OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(_start)

PHDRS
{
	text PT_LOAD FILEHDR PHDRS;
	rm16 PT_LOAD;
}

SECTIONS
{
	. = 0x301000 + SIZEOF_HEADERS;

	.text : {
		*(.text .stub .text.* .gnu.linkonce.t.*)
		*(.gnu.warning)
		PROVIDE(_etext = .);

		. = ALIGN(0x10);
		*(.rodata .rodata.* .gnu.linkonce.r.*)

		. = ALIGN(0x10);
		*(.data .data.* .gnu.linkonce.d.*)
		PROVIDE(_edata = .);

		. = ALIGN(0x10);
	} :text

	.bss : {
		*(.bss .bss.* .gnu.linkonce.b.*)
		*(COMMON)
		PROVIDE(_end = .);
	} :text

	. = .;

	HIDDEN(PAGESIZE = CONSTANT(COMMONPAGESIZE));
	rm16_run_start = ALIGN(_edata, 0x10) % PAGESIZE;

	.rm16 rm16_run_start : AT(ALIGN(_end, PAGESIZE) + rm16_run_start) {
		*(.rm16.text .rm16.stub .rm16.text.*)

		. = ALIGN(0x10);
		*(.rm16.rodata .rm16.rodata.*)

		. = ALIGN(0x10);
		*(.rm16.data .rm16.data.*)

		. = ALIGN(0x10);
		*(.rm16.bss .rm16.bss.*)
	} :rm16

	rm16_load_start = LOADADDR(.rm16);
	rm16_load_dwords = (. - rm16_run_start) / 4;

	/DISCARD/ : {
		*(.note.GNU-stack .gnu_* .gnu.* .stab* .debug_* .eh_frame
		  .comment)
	}
}
