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

	bits	16

	extern	StackPos, rm16_call.fin1, ps2_keyboard_setup, zonelow_seg_var

	global	rm16_call.cont1, rm16_call.rm_cs16
rm16_call.cont1:			; on entry eax, ebx, ecx, edx give
					; the parameters to pass to the
					; callee, [edi] is the real mode far
					; address to call, & esi is free
	mov	si, SEL_DS16_ZERO	; prime segment descriptor caches
	mov	ds, si			; with 16-bit properties
	mov	es, si
	mov	ss, si
	mov	fs, si
	mov	gs, si
	mov	esi, cr0		; switch to real mode without paging
	and	esi, ~(CR0_PG|CR0_PE)
	mov	cr0, esi
	jmp	0:rm16_call.cont2
rm16_call.rm_cs16 equ $-2
rm16_call.cont2:
	mov	si, [cs:zonelow_seg_var] ; really set up segments
	mov	ds, si
	mov	es, si
	mov	esi, esp
	cmp	esi, BMEM_MAX_ADDR	; if stack resides in base memory,
	jna	rm16_call.exist_stack	; then use it --- (1)
	mov	sp, ds			; otherwise switch to a base memory
	mov	ss, sp			; stack
	mov	esp, [StackPos]
rm16_call.save_old_stack:
	push	esi			; save old esp
	xor	si, si
	mov	fs, si
	mov	gs, si
	mov	esi, cr4		; turn off cr4.PAE in case some 3rd-
	and	si, byte ~CR4_PAE	; -party code wants to set up its
	mov	cr4, esi		; own page tables at some point...
	call	far word [fs:edi]	; call the callee
	cli
	pop	esp			; restore the 32-bit esp
	mov	ds, [cs:zonelow_seg_var] ; restore our GDTR
	lgdt	[gdtr]
	mov	eax, cr0		; return to 32-bit protected mode;
	or	al, CR0_PE		; eax := cr0
	mov	cr0, eax
	jmp	SEL_CS32:dword rm16_call.fin1

rm16_call.exist_stack:			; (1) --- if stack resides in first
	cmp	esi, 0x10000		; 64 KiB, just use ss := 0
	jna	rm16_call.low_low_stack
	shr	esp, 4			; otherwise, set ss to 64 KiB above
	sub	sp, 0x1000		; the current esp, then set esp := 0
	mov	ss, sp
	xor	sp, sp
	jmp	rm16_call.save_old_stack
rm16_call.low_low_stack:
	xor	sp, sp
	mov	ss, sp
	movzx	esp, si
	jmp	rm16_call.save_old_stack

	global	hello16
hello16:
	mov	ax, 0x0003
	int	0x10
	mov	ah, 0x03
	xor	bh, bh
	int	0x10
	mov	ax, 0x1301
	mov	bx, 0x0007
	mov	cx, msg.end-msg
	mov	bp, msg
	int	0x10
	retf

	section	.rodata

	extern	gdt, GDT_SZ

	global	gdtr
gdtr:	dw	GDT_SZ-1
	dd	gdt

msg:	db	".:. biefircate ", VERSION, " .:. "
	db	"hello world from int 0x10", 13, 10
.end:
