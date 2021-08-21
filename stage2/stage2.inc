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
PIC1_DATA equ	0x0021
PIC2_DATA equ	0x00a1

; Segment selector values for our GDT.
SEL_CS32 equ	0x0008
SEL_DS32 equ	0x0010
SEL_CS16 equ	0x0018
SEL_DS16_ZERO equ 0x0020

; BIOS data area variables.
	absolute 0x0400
bda:
.com1:	resw	1			; 0x0400
.com2:	resw	1			; 0x0402
.com3:	resw	1			; 0x0404
.com4:	resw	1			; 0x0406
.lpt1:	resw	1			; 0x0408
.lpt2:	resw	1			; 0x040a
.lpt3:	resw	1			; 0x040c
.ebda:	resw	1			; 0x040e
.eqpt:	resw	1			; 0x0411
.post_flag: resb 1			; 0x0412
.base_kib: resw	1			; 0x0413

%endif
