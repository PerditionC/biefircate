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

/* GNU ld compatible linker script for the 16-bit portion of stage 2. */

OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(_stext16)

SECTIONS
{
	HIDDEN(KIBYTE = 1024);

	.data 0 : {
		_sdata16 = .;

		*(.rodata .rodata.* .gnu.linkonce.r.*)

		. = ALIGN(0x10);
		*(.data .data.* .gnu.linkonce.d.*)

		. = ALIGN(0x10);
		_edata16 = .;
	}

	.bss : {
		*(.bss .bss.* .gnu.linkonce.b.*)
		*(COMMON)

		. = ALIGN(KIBYTE);
		_end16 = .;
	}

	. = .;
	_END16_KIB = . / KIBYTE;

	.text 0 : AT(ALIGN(. + 1, CONSTANT(COMMONPAGESIZE))) {
		_stext16 = .;

		*(.text .stub .text.* .gnu.linkonce.t.*)
		*(.gnu.warning)

		_etext16 = .;
	}

	/DISCARD/ : {
		*(.note.GNU-stack .gnu_* .gnu.* .stab* .debug_* .eh_frame
		  .comment)
	}
}
