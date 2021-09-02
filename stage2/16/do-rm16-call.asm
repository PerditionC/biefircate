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

	extern	_stack16, ps2_keyboard_setup

	global	rm16_call.cont1, rm16_call.rm_cs16
rm16_call.cont1:			; on entry eax, ebx, ecx, edx give
					; the parameters to pass to the
					; callee, [edi] is the real mode far
					; address to call, & esi is free
	mov	si, SEL_DS16_ZERO	; prime segment descriptor caches
	mov	ds, si			; with 16-bit properties
	mov	es, si
	mov	fs, si
	mov	gs, si
	mov	esi, cr0		; switch to real mode without paging
	and	esi, ~(CR0_PG|CR0_PE)
	mov	cr0, esi
	jmp	0:rm16_call.cont2
rm16_call.rm_cs16 equ $-2
rm16_call.cont2:
	mov	si, [bda.ebda]		; really set up segments
	mov	ds, si
	mov	[sp32], esp		; store the protected-mode esp & cr3
	mov	esp, cr3
	mov	[ptpd32], esp
	mov	ss, si
	mov	esp, _stack16
	mov	es, si
	xor	si, si
	mov	fs, si
	mov	gs, si
	sgdt	[gdtr]			; save our GDTR
	mov	esi, cr4		; turn off cr4.PAE in case some 3rd-
	and	si, byte ~CR4_PAE	; -party code wants to set up its
	mov	cr4, esi		; own page tables at some point...
	call	far word [fs:edi]	; call the callee
	cli
	xor	si, si			; restore the 32-bit stack & also
	mov	ss, si			; restore our GDTR, esp, & cr3
	mov	ds, [ss:bda.ebda]
	mov	esp, [sp32]
	mov	esi, [ptpd32]
	mov	cr3, esi
	lgdt	[gdtr]
	mov	esi, cr4		; return to 32-bit protected mode
	or	si, byte CR4_PAE	; with PAE paging
	mov	cr4, esi
	mov	esi, cr0
	or	esi, CR0_PG|CR0_PE
	mov	cr0, esi
	jmp	short rm16_call.cont3
rm16_call.cont3:
	add	esp, 8
	jmp	far dword [esp-8]

	global	hello16
hello16:
	xor	eax, eax
	call	dword ps2_keyboard_setup
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
	sti
	jmp	$
	retf

	section	.rodata

msg:	db	".:. biefircate ", VERSION, " .:. "
	db	"hello world from int 0x10", 13, 10
.end:

	section	.bss

sp32:	resd	1
ptpd32:	resd	1
gdtr:	resb	6
