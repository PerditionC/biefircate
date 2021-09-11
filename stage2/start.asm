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

%include "stage2/stage2.inc"

	section	.text

	extern	mem_init, stage2_main, rm16, gdtr_load_time

	global	_start
_start:
	cli
	cld
	mov	esp, starting_stack
	lgdt	[gdtr_load_time]
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
	call	stage2_main

	section .rodata

idtrrm:	dw	0x100*4-1
	dd	0

	section	.data

	global	gdt, gdt_desc_cs16, GDT_SZ

	align	8
gdt	equ	$-8
%if SEL_CS32 != $-gdt
%   error "SEL_CS32 does not match actual GDT"
%endif
	dq	0x00cf9a000000ffff	; 32-bit protected mode code seg.
%if SEL_DS32 != $-gdt
%   error "SEL_DS32 does not match actual GDT"
%endif
	dq	0x00cf92000000ffff	; 32-bit protected mode data seg.
%if SEL_CS16 != $-gdt
%   error "SEL_CS16 does not match actual GDT"
%endif
gdt_desc_cs16:
	dq	0x008f9a000000ffff	; 16-bit protected mode code seg.
					; pointing to our 16-bit code
%if SEL_DS16_ZERO != $-gdt
%   error "SEL_DS16_ZERO does not match actual GDT"
%endif
	dq	0x008f92000000ffff	; 16-bit protected mode data seg.
					; pointing to linear address zero
GDT_SZ	equ	$-gdt

	section	.bss

	resb	0x1000
starting_stack:
