; Copyright (c) 2021 TK Chia
;
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the following conditions are
; met:
;
;   * Redistributions of source code must retain the above copyright
;     notice, this list of conditions and the following disclaimer.
;   * Redistributions in binary form must reproduce the above copyright
;     notice, this list of conditions and the following disclaimer in the
;     documentation and/or other materials provided with the distribution.
;   * Neither the name of the developer(s) nor the names of its
;     contributors may be used to endorse or promote products derived from
;     this software without specific prior written permission.
;
; THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
; IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
; TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
; PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
; HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
; SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
; TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
; PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
; LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
; NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
; SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

	section	.text

%define MAGIC32(a, b, c, d) \
	(((a) & 0xff)	    | \
	 ((b) & 0xff) <<  8 | \
	 ((c) & 0xff) << 16 | \
	 ((d) & 0xff) << 24)

VGA_INIT_SEG equ 0x1000

	extern	mem_init, rm16_load_start, rm16

	global	_start
_start:
	mov	esp, starting_stack
	lgdt	[gdtr]
	lidt	[idtrrm]
	jmp	SEL_CS32:.cont
.cont:
	mov	ax, SEL_DS32
	mov	ds, ax
	mov	es, ax
	mov	ss, ax
	mov	fs, ax
	mov	gs, ax
	mov	eax, ebp
	call	mem_init
	mov	edi, VGA_INIT_SEG<<4
	mov	esi, rm16_load
	mov	ecx, (rm16_load.end-rm16_load)/4
	cld
	rep movsd
	mov	ax, SEL_DS16
	jmp	SEL_CS16:word rm16

	section .rodata

	align	8
	global	SEL_CS32, SEL_DS32, SEL_CS16, SEL_DS16
gdt equ		$-8
SEL_CS32 equ	$-gdt
	dq	0x00cf9a000000ffff	; 32-bit protected mode code seg.
SEL_DS32 equ	$-gdt
	dq	0x00cf92000000ffff	; 32-bit protected mode data seg.
SEL_CS16 equ	$-gdt
					; 16-bit protected mode code seg.
	dq	0x008f9a000000ffff|(VGA_INIT_SEG<<4<<16)
SEL_DS16 equ	$-gdt
	dq	0x008f92000000ffff	; 16-bit protected mode data seg.
gdt_end:
gdtr:	dw	gdt_end-gdt-1
	dd	gdt
idtrrm:	dw	0x100*4-1
	dd	0

	align	4
rm16_load:
	incbin	"stage2/16.bin"
	align	4
.end:

	section	.bss

	resb	0x1000
starting_stack:
