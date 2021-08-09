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

	extern	mem_init, rm16_load_start, rm16_load_dwords

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
	mov	esi, rm16_load_start
	mov	ecx, rm16_load_dwords
	cld
	rep movsd
	jmp	SEL_CS16:word 0

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

	section	.rm16.text

	bits	16
	mov	ax, SEL_DS16		; prime segment descriptor caches
	mov	ds, ax
	mov	es, ax
	mov	ss, ax
	mov	fs, ax
	mov	gs, ax
	mov	esp, 0x0600
	mov	eax, cr0		; switch to real mode
	and	al, 0b11111110
	mov	cr0, eax
	jmp	VGA_INIT_SEG:word cont
cont:
	xor	ax, ax			; really set up segments
	mov	ds, ax
	mov	es, ax
	mov	ss, ax
	mov	fs, ax
	mov	gs, ax
	jmp	find
next:
	mov	ebp, [ebp]
find:
					; look for a "PCID" boot param. with
					; an option ROM
	cmp	dword [ebp+8], MAGIC32('P', 'C', 'I', 'D')
	jnz	next
	mov	cx, [ebp+0x1c]
	jcxz	next
	mov	ax, [ebp+0x10]		; get VGA controller's PCI locn.
	mov	bx, [ebp+0x1e]		; set the dest. seg. for the opt. ROM
	push	word [ebp+0x1c]		; call the option ROM code
	push	word 3
	call	far word [esp]
	pop	eax
	mov	ax, 0x0003		; try to set 80 * 25 screen mode
	int	0x10
	mov	si, msg
say:
	cs lodsb
	test	al, al
	jz	stop
	mov	ah, 0xe
	mov	bx, 0x0007
	int	0x10
	jmp	say
stop:
	cli
	hlt				; snooze

msg:	db	"Hello world from int 0x10!", 13, 10, 0

	align	16

	section	.bss

	resb	0x2000
starting_stack:
