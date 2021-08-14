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

; Dummy legacy master boot record code which directs the user to restart the
; system in UEFI mode.

	org	0x7c00

	global	_start
_start:
	jmp	0:.cont			; set up segment registers
.cont:
	xor	ax, ax
	cli
	mov	ss, ax
	mov	sp, _start
	sti
	mov	ds, ax
	mov	si, msg			; display the error message
	mov	di, msg.end-msg
.loop:
	lodsb
	mov	ah, 0x0e
	mov	bx, 0x0007
	int	0x10
	dec	di
	jnz	.loop
	xor	ah, ah			; wait for a keystroke
	int	0x16
	push	sp			; test if we are on a 286 or above
	pop	ax
	cmp	ax, sp
	cli
	jnz	.no_286
	lidt	[bad_idtr]		; if we are, use a triple fault to
	int3				; reset the PC
.no_286:
	jmp	0xf000:0xfff0		; otherwise, jump to the reset code

	align	2

bad_idtr:
	dw	0

msg:	db	13, 10
	db	"System started in legacy BIOS mode", 13, 10
	db	"Please restart in UEFI mode", 13, 10, 7
.end:

	times	0x200-2-($-$$) db 0

	db	0x55, 0xaa
