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

; Interrupt vector table entries.

%include "stage2/stage2.inc"

	bits	16

	extern	zonelow_seg_var, StackPos

; Begin the table of vectors.
%macro	ISR_START 0
	section	.rodata
	global	vecs16
vecs16:
%endmacro

; Add a vector table entry for an unimplemented interrupt.
%macro	ISR_UNIMPL 1
	section	.text
isr16_%{1}_unimpl:
	call	isr16_unimpl
	db	%1
	section	.rodata
	dw	isr16_%{1}_unimpl
%endmacro

; Add a vector table entry for a software interrupt implemented in assembly.
%macro	ISR_IMPL 1
	section	.rodata
	dw	isr16_%1
%endmacro

; Add a vector table entry for a software interrupt implemented in C.
%macro	ISR_IMPL_C 1
	section	.text
	extern	isr16_%1_impl
isr16_%1:
	push	eax			; save registers as per `struct bregs'
	push	ecx
	push	edx
	push	ebx
	push	ebp
	push	esi
	push	edi
	push	es
	push	ds
	push	fs
	push	gs
	mov	ax, ss			; set ds := ss, so C code can access
	mov	ds, ax			; stack via ds
	xor	ax, ax			; set gs := 0, fs := our data segment
	mov	gs, ax
	mov	fs, word [cs:zonelow_seg_var]
	mov	eax, esp		; align esp to 4-byte boundary, clear
	mov	ebx, esp		; its top 16 bits, save the old esp
	and	esp, 0xffff&-4		; in ebx, & pass the old esp (for
					; the isr16_regs_t) as a parm. in eax
	call	dword isr16_%1_impl	; call the C routine
	mov	esp, ebx		; restore esp & retrieve other
	pop	gs			; registers
	pop	fs
	pop	ds
	pop	es
	pop	edi
	pop	esi
	pop	ebp
	pop	ebx
	pop	edx
	pop	ecx
	pop	eax
	iret				; return to caller (& pop flags)
	section	.rodata
	dw	isr16_%1
%endmacro

; Add a vector table entry for a software interrupt implemented in C in
; SeaBIOS.
%macro	ISR_IMPL_SBIOS 2
	section	.text
	extern	%2
isr16_%1:
	push	eax			; save registers as per `struct bregs'
	push	ecx
	push	edx
	push	ebx
	push	ebp
	push	esi
	push	edi
	push	es
	push	ds
	mov	ax, ss			; set ds := ss, so C code can access
	mov	ds, ax			; stack via ds
	mov	eax, esp		; align esp to 4-byte boundary, clear
	mov	ebx, esp		; its top 16 bits, save the old esp
	and	esp, 0xffff&-4		; in ebx, & pass the old esp (for
					; the isr16_regs_t) as a parm. in eax
	call	dword %2		; call the C routine
	mov	esp, ebx		; restore esp & retrieve other
	pop	ds			; registers
	pop	es
	pop	edi
	pop	esi
	pop	ebp
	pop	ebx
	pop	edx
	pop	ecx
	pop	eax
	iret				; return to caller (& pop flags)
	section	.rodata
	dw	isr16_%1
%endmacro

; Add a vector table entry for an IRQ implemented in assembly.
%macro	ISR_IRQ	2
	section	.rodata
	dw	irq%2
%endmacro

; Add a vector table entry for an IRQ implemented in C.
%macro	ISR_IRQ_C 3
	section	.text
	extern	%3
irq%2:
	push	eax			; save call-used registers
	push	ecx
	push	edx
	push	ds			; save segment registers
	push	es
	push	fs
	push	gs
	mov	ax, ss			; set ds := ss, so C code can access
	mov	ds, ax			; stack via ds
	xor	ax, ax			; set gs := 0, fs := our data segment
	mov	gs, ax
	mov	fs, word [cs:zonelow_seg_var]
	mov	eax, esp		; align esp to 4-byte boundary, clear
	and	esp, 0xffff&-4		; its top 16 bits, & save old esp
	push	eax
	call	dword %3		; call the C routine
	pop	esp			; restore esp & other registers
	pop	gs
	pop	fs
	pop	es
	pop	ds
	pop	edx
	pop	ecx
	pop	eax
	iret				; return to interrupted program
	section	.rodata
	dw	irq%2
%endmacro

; Add a vector table entry for an interrupt which does an `iret' on default.
%macro	ISR_IRET 1
	section	.rodata
	dw	iret16
%endmacro

; End the table of vectors.
%macro	ISR_END 0
	section	.rodata
	global	NUM_VECS16
NUM_VECS16 equ	($-vecs16)/2
%endmacro

	extern	irq0, isr16_0x1a

	ISR_START
	ISR_UNIMPL 0x00
	ISR_IRET 0x01
	ISR_UNIMPL 0x02
	ISR_IRET 0x03
	ISR_IRET 0x04
	ISR_UNIMPL 0x05
	ISR_UNIMPL 0x06
	ISR_UNIMPL 0x07
	ISR_IRQ 0x08, 0
	ISR_IRQ_C 0x09, 1, handle_09
	ISR_UNIMPL 0x0a
	ISR_UNIMPL 0x0b
	ISR_UNIMPL 0x0c
	ISR_UNIMPL 0x0d
	ISR_UNIMPL 0x0e
	ISR_UNIMPL 0x0f
	ISR_UNIMPL 0x10
	ISR_IMPL 0x11
	ISR_IMPL 0x12
	ISR_IMPL_SBIOS 0x13, handle_13
	ISR_IRET 0x14  ; FIXME
	ISR_IMPL_C 0x15
	ISR_IMPL_SBIOS 0x16, handle_16
	ISR_UNIMPL 0x17
	ISR_UNIMPL 0x18
	ISR_UNIMPL 0x19
	ISR_IMPL 0x1a
	ISR_IRET 0x1b
	ISR_IRET 0x1c
	ISR_END

	section	.text

; Handler for int 0x11 (get equipment list).
isr16_0x11:
	push	ds
	xor	ax, ax
	mov	ds, ax
	movzx	eax, word [bda.eqpt]
	pop	ds
iret16:	iret

; Handler for int 0x12 (get memory size).
isr16_0x12:
	push	ds
	xor	ax, ax
	mov	ds, ax
	mov	ax, [bda.base_kib]
	pop	ds
	iret

	extern	_stack16

; Catch-all for unimplemented interrupt service routines.
isr16_unimpl:
	pop	bx
	xchg	dx, ax			; save our incoming ax
	mov	al, ~0			; mask all IRQs --- frob the PICs
	out	PIC1_DATA, al		; so that even if `int 0x10' uses
	out	PIC2_DATA, al		; `sti', no IRQs will trigger
	mov	ax, [cs:zonelow_seg_var] ; switch to our stack
	mov	ds, ax
	mov	es, ax
	mov	ss, ax
	mov	esp, [StackPos]
	mov	al, [cs:bx]		; plug in the interrupt vector no.
	call	u8_to_hex
	mov	[msg_unimpl.num], ax
	mov	al, dh			; plug in the incoming ax
	call	u8_to_hex
	mov	[msg_unimpl.ax], ax
	xchg	dx, ax
	call	u8_to_hex
	mov	[msg_unimpl.ax+2], ax
	mov	ah, 0x03		; then really panic
	xor	bh, bh
	int	0x10
	mov	ax, 0x1301
	mov	bx, 0x0007
	mov	cx, msg_unimpl.end-msg_unimpl
	mov	bp, msg_unimpl
	int	0x10
	cli
	hlt

; Convert an 8-bit binary value in al, to its hexadecimal representation in
; ASCII in al:ah.
u8_to_hex:
	mov	ah, al
	shr	al, 4
	and	ah, 0x0f
	add	al, '0'
	cmp	al, '9'
	jbe	.0
	add	al, 'a'-('9'+1)
.0:	add	ah, '0'
	cmp	ah, '9'
	jbe	.1
	add	ah, 'a'-('9'+1)
.1:	ret

	section	.data

msg_unimpl:
	db	13, 10, "stage2 panic: int 0x"
.num:	db	"00 unimplemented (with ax = 0x"
.ax:	db	"0000)", 7
.end:
