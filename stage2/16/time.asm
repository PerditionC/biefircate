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

	section	.text

; IRQ 0 (system timer) handler.
	global	irq0
irq0:
	push	ds
	push	eax
	xor	ax, ax
	mov	ds, ax
	mov	eax, [bda.timer]	; increment timer tick count
	inc	eax
	cmp	eax, 0x1800b0		; if 24 hours (or more) since
	jae	.ovf			; midnight, increment overflow byte
.cont:
	mov	[bda.timer], eax
	int	0x1c			; invoke user (?) timer tick handler
	mov	al, OCW2_EOI		; send EOI to first PIC
	out	PIC1_CMD, al
	pop	eax
	pop	ds
	iret
.ovf:
	inc	byte [bda.timer_ovf]
	xor	eax, eax
	jmp	.cont

; Handler for int 0x1a.
	global	isr16_0x1a
isr16_0x1a:
	cmp	ah, (.hndl_end-.hndl)/2
	jae	.not_time_fn
	push	si
	movzx	si, ah
	shl	si, 1
	jmp	word [cs:.hndl+si]
; Function 0x00: get system time & midnight flag.
.fn0x00:
	push	ds
	xor	dx, dx
	mov	ds, dx
	mov	cx, [bda.timer+2]
	mov	dx, [bda.timer]
	mov	al, 0
	xchg	al, [bda.timer_ovf]
	pop	ds
.iret:
	pop	si
	iret
; Function 0x01: set system time.
.fn0x01:
	cmp	cx, 0x18
	ja	.iret
	jb	.ok0x01
	cmp	dx, 0x00b0
	jae	.iret
.ok0x01:
	push	ds
	push	byte 0
	pop	ds
	mov	[bda.timer+2], cx
	mov	[bda.timer], dx
	pop	ds
	pop	si
	iret
; Function 0x02: get RTC time.
.fn0x02:
	call	is_rtc_ok
	jnz	.error
	push	ax
	push	bx
	push	si
	xor	si, si
.wait0x02:
	call	read_rtc		; read the RTC until we get a stable
	xchg	cx, ax			; value
	mov	bx, dx
	call	read_rtc
	cmp	ax, cx
	jnz	.wait0x02
	cmp	bx, dx
	jz	.ok0x02
.retry0x02:
	dec	si			; if we tried too many times & still
	jnz	.wait0x02		; get inconsistent readings...
	pop	bx
	pop	ax
.error:
	pop	si
	stc
	jmp	.done
.ok0x02:				; otherwise, return success
	pop	bx
	pop	ax
	clc
.done:
	sti
	retf	2
.not_time_fn:
	; TODO
	stc
	jmp	.done

.hndl:	dw	.fn0x00, .fn0x01, .fn0x02
.hndl_end:

is_rtc_ok:
	push	ax
	mov	al, CMOS_DIAG
	call	read_cmos
	test	al, DIAG_BAD_BAT|DIAG_BAD_CKSUM|DIAG_BAD_CLK
	pop	ax
	ret

read_rtc:
	push	ax
	push	cx
	xor	cx, cx
.wait_for_no_upd:			; wait for RTC to stop updating
	mov	al, CMOS_RTC_STA_A
	call	read_cmos
	test	al, RTC_A_UIP
	loopnz	.wait_for_no_upd
	jnz	.error			; if timed out, then bail out
	pop	cx			; otherwise, read off the time
	mov	al, CMOS_RTC_STA_B
	call	read_cmos
	and	al, RTC_B_DST
	xchg	dx, ax
	mov	al, CMOS_RTC_SEC
	call	read_cmos
	mov	dh, al
	mov	al, CMOS_RTC_MIN
	call	read_cmos
	xchg	cx, ax
	mov	al, CMOS_RTC_HR
	call	read_cmos
	mov	ch, al
	pop	ax
	clc
	ret
.error:
	pop	cx
	pop	ax
	stc
	ret

read_cmos:
	or	al, CMOS_NMI_DIS
	out	PORT_CMOS_IDX, al
	out	PORT_DUMMY, al
	in	al, PORT_CMOS_DATA
	push	ax
	mov	al, CMOS_RTC_STA_D
	out	PORT_CMOS_IDX, al
	out	PORT_DUMMY, al
	pop	ax
	ret
