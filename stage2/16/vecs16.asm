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

	bits	16

%define	TO_HEX_DIGIT(v)	((v)>9?(v)-10+'a':(v)+'0')

%macro	ISR_UNIMPL 1
	section	.text
isr16_%{1}_unimpl:
	call	isr16_unimpl
	db	TO_HEX_DIGIT((%1)>>4), TO_HEX_DIGIT((%1)&0x0f)
	section	.rodata
    %if (%1) == 0x00
	global	vecs16
vecs16:
    %endif
	dw	isr16_%{1}_unimpl
%endmacro

%macro	ISR_IMPL 1
	section	.rodata
	dw	isr16_%1
%endmacro

%macro	ISR_IRET 1
	section	.rodata
	dw	iret16
%endmacro

%macro	ISR_END 0
	section	.rodata
	global	NUM_VECS16
NUM_VECS16 equ	($-vecs16)/2
%endmacro

	ISR_UNIMPL 0x00
	ISR_IRET 0x01
	ISR_UNIMPL 0x02
	ISR_IRET 0x03
	ISR_IRET 0x04
	ISR_UNIMPL 0x05
	ISR_UNIMPL 0x06
	ISR_UNIMPL 0x07
	ISR_UNIMPL 0x08
	ISR_UNIMPL 0x09
	ISR_UNIMPL 0x0a
	ISR_UNIMPL 0x0b
	ISR_UNIMPL 0x0c
	ISR_UNIMPL 0x0d
	ISR_UNIMPL 0x0e
	ISR_UNIMPL 0x0f
	ISR_UNIMPL 0x10
	ISR_IMPL 0x11
	ISR_IMPL 0x12
	ISR_UNIMPL 0x13
	ISR_UNIMPL 0x14
	ISR_UNIMPL 0x15
	ISR_UNIMPL 0x16
	ISR_UNIMPL 0x17
	ISR_UNIMPL 0x18
	ISR_UNIMPL 0x19
	ISR_UNIMPL 0x1a
	ISR_IRET 0x1b
	ISR_IRET 0x1c
	ISR_END

	section	.text

isr16_0x11:
	push	ds
	xor	ax, ax
	mov	ds, ax
	movzx	eax, word [bda.eqpt]
	pop	ds
iret16:	iret

isr16_0x12:
	push	ds
	xor	ax, ax
	mov	ds, ax
	mov	ax, [bda.base_kib]
	pop	ds
	iret

	extern	_stack16

isr16_unimpl:
	pop	bx
	mov	al, ~0
	out	PIC1_DATA, al
	out	PIC2_DATA, al
	xor	ax, ax
	mov	ds, ax
	mov	ax, [bda.ebda]
	mov	ds, ax
	mov	es, ax
	mov	ss, ax
	mov	sp, _stack16
	mov	ax, [cs:bx]
	mov	[msg_unimpl.num], ax
	mov	ah, 0x03
	xor	bh, bh
	int	0x10
	mov	ax, 0x1301
	mov	bx, 0x0007
	mov	cx, msg_unimpl.end-msg_unimpl
	mov	bp, msg_unimpl
	int	0x10
	cli
	hlt

	section	.data

msg_unimpl:
	db	13, 10, "stage2 panic: int 0x"
.num:	db	"00 unimplemented", 7
.end:
