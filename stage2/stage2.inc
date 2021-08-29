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

%ifndef	_H_STAGE2
%define	_H_STAGE2

%include "common.inc"

; Legacy 8259 programmable interrupt controller (PIC) I/O port numbers.
PIC1_CMD equ	0x0020
PIC1_DATA equ	0x0021
PIC2_CMD equ	0x00a0
PIC2_DATA equ	0x00a1

; OCW2 bit fields for the PICs.
OCW2_EOI equ	0x20			; non-specific EOI

; CMOS port numbers.
PORT_CMOS_IDX equ 0x0070
PORT_CMOS_DATA equ 0x0071

; CMOS register indices & flags.
CMOS_RTC_SEC equ 0x00			; real-time clock (RTC) seconds
CMOS_RTC_SEC_ALRM equ 0x01		; RTC second alarm
CMOS_RTC_MIN equ 0x02			; RTC minutes
CMOS_RTC_MIN_ALRM equ 0x03		; RTC minute alarm
CMOS_RTC_HR equ 0x04			; RTC hours
CMOS_RTC_HR_ALRM equ 0x05		; RTC hour alarm
CMOS_RTC_STA_A equ 0x0a			; status register A
CMOS_RTC_STA_B equ 0x0b			; status register B
CMOS_RTC_STA_C equ 0x0c			; status register C
CMOS_RTC_STA_D equ 0x0d			; status register D
CMOS_DIAG equ	0x0e			; diagnostic status
CMOS_NMI_DIS equ 0x80			; flag to disable NMIs

; Bit fields in RTC status register A.
RTC_A_UIP equ	0x80			; update in progress

; Bit fields in RTC status register B.
RTC_B_DST equ	0x01			; daylight saving time

; Bit fields in CMOS diagnostic status byte.
DIAG_BAD_BAT equ 0x80			; clock lost power
DIAG_BAD_CKSUM equ 0x40			; incorrect CMOS checksum
DIAG_BAD_CLK equ 0x04			; RTC time is invalid

; Other ports.
PORT_DUMMY equ	0x0080

; Segment selector values for our GDT.
SEL_CS32 equ	0x0008
SEL_DS32 equ	0x0010
SEL_CS16 equ	0x0018
SEL_DS16_ZERO equ 0x0020

; BIOS data area variables.
	absolute 0x0400
bda:
.com1:	resw	1			; 0x40:0x00: 1st serial port base addr.
.com2:	resw	1			; 0x40:0x02: 2nd serial port base addr.
.com3:	resw	1			; 0x40:0x04: 3rd serial port base addr.
.com4:	resw	1			; 0x40:0x06: 4th serial port base addr.
.lpt1:	resw	1			; 0x40:0x08: 1st parallel port base
.lpt2:	resw	1			; 0x40:0x0a: 2nd parallel port base
.lpt3:	resw	1			; 0x40:0x0c: 3rd parallel port base
.ebda:	resw	1			; 0x40:0x0e: ext. BIOS data area seg.
.eqpt:	resw	1			; 0x40:0x10: installed hardware
	resb	1
.base_kib: resw	1			; 0x40:0x13: base memory size in KiB
	resb	2
.kb_stat1: resb	1			; 0x40:0x17: status flag 1
.kb_stat2: resb 1			; 0x40:0x18: status flag 2
.kb_keypad: resb 1			; 0x40:0x19: Alt-nnn keypad workspace
.kb_buf_head: resw 1			; 0x40:0x1a: head in kbd. buf. queue
.kb_buf_tail: resw 1			; 0x40:0x1c: tail in kbd. buf. queue
.def_kb_buf: resw 16			; 0x40:0x1e: default keyboard buffer
.def_kb_buf_end:
.fd_recalib: resb 1			; 0x40:0x3e: floppy recalibrate status
.fd_motor: resb	1			; 0x40:0x3f: floppy motor status
.fd_cntdn: resb 1			; 0x40:0x40: floppy motor turn-off
					;	     timeout count
.fd_error: resb	1			; 0x40:0x41: floppy last op. status
.dsk_status: resb 7			; 0x40:0x42: floppy/hard drive status/
					;	     command bytes
.vid_mode: resb	1			; 0x40:0x49: video mode
.vid_cols: resw	1			; 0x40:0x4a: columns on screen
.vid_page_sz: resw 1			; 0x40:0x4c: video page size
.vid_page_start: resw 1			; 0x40:0x4e: video page start addr.
.vid_xy: resw	8			; 0x40:0x50: cursor positions in
					;	     each of 8 video pages
.vid_curs_shape: resw 1			; 0x40:0x60: cursor shape
.vid_pg: resb	1			; 0x40:0x62: video page no.
.crtc:	resw	1			; 0x40:0x63: CRT base I/O port addr.
					;	     (normally 0x03b4 or
					;	     0x03d4)
.vid_msr: resb	1			; 0x40:0x65: last value written to
					;	     port 0x03b8 or 0x03d8
.vid_pal: resb	1			; 0x40:0x66: last value written to
					;	     port 0x03d9
.restart: resd	1			; 0x40:0x67: reset restart addr.
.stray_irq: resb 1			; 0x40:0x6b: IRQ in-service reg.
					;	     value from time of last
					;	     stray IRQ
.timer:	resd	1			; 0x40:0x6c: timer ticks since midnight
.timer_ovf:  resb 1			; 0x40:0x70: timer overflow
.ctrlc:	resb	1			; 0x40:0x71: Ctrl-Break flag
.reset_flag: resw 1			; 0x40:0x72: POST reset flag
.hd_error: resb	1			; 0x40:0x74: fixed disk last op. stat.
.hd_cnt: resb	1			; 0x40:0x75: no. of fixed disk drives
.hd_ctl: resb	1			; 0x40:0x76: fixed disk ctrl. byte
.hd_port_off: resb 1			; 0x40:0x77: fixed disk I/O port off.
.lpt1_cntdn: resb 1			; 0x40:0x78: par. dev. 1 timeout cnt.
.lpt2_cntdn: resb 1			; 0x40:0x79: par. dev. 2 timeout cnt.
.lpt3_cntdn: resb 1			; 0x40:0x7a: par. dev. 3 timeout cnt.
.flags_0x4b: resb 1			; 0x40:0x7b: int 0x4b flags
.com1_cntdn: resb 1			; 0x40:0x7c: ser. dev. 1 timeout cnt.
.com2_cntdn: resb 1			; 0x40:0x7d: ser. dev. 2 timeout cnt.
.com3_cntdn: resb 1			; 0x40:0x7e: ser. dev. 3 timeout cnt.
.com4_cntdn: resb 1			; 0x40:0x7f: ser. dev. 4 timeout cnt.
.kb_buf_start: resw 1			; 0x40:0x80: kbd. buf. start offset
.kb_buf_end: resw 1			; 0x40:0x82: kbd. buf. end offset

%endif
