; Copyright (c) 2020--2021 TK Chia
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

	section .text

	global	run_stage2
run_stage2:
	; on entry:
	; ecx = ELF entry point for stage 2
	; edx -> space for 32-bit trampoline
	; r8d = size of base memory block starting at address 0, in KiB
	; r9d = real mode segment of temporary EBDA
	; [rsp+8+0x20] = VGA controller's PCI locn. (<bus, device, function>)
	; [rsp+8+0x28] = real mode seg. of copy of VGA controller's option ROM
	cli
	mov	eax, [rsp+8+0x20]	; get VGA's PCI location
	mov	ebx, [rsp+8+0x28]	; get VGA option ROM address
	lea	esp, [edx+0x1000]	; switch to new stack
	push	rax			; push VGA's PCI locn. & opt. ROM addr.
	mov	[esp+4], ebx
	mov	ecx, ecx		; push ELF32 entry point
	push	rcx
	push	r8			; push base mem. size & temp. EBDA seg.
	mov	[esp+4], r9d
	xor	edi, edi		; clear the real mode interrupt
	mov	ecx, 0x0600/8		; vector table & BIOS data area
	xor	eax, eax
	rep stosq
	mov	edi, edx
	push	rdi			; push trampoline address
	lea	rsi, [rel LB]		; copy trampoline to mem. < 4 GiB
	mov	ecx, (LE-LB)/8
	rep movsq
	add	[Lpm32_gdtr+2-LE+rdi], edx  ; fix up GDTR value
	retq				; jump to trampoline

	align	8
LB:					; <<< start of trampoline to copy >>>
	lgdt	[Lpm32_gdtr-LE+rdi]	; go to 32-bit protected mode
	push	8
	lea	rdi, [Lpm32-LE+rdi]
	push	rdi
	retfq

	bits	32
Lpm32:
	mov	ax, 0x10		; prime segment descriptor caches
	mov	ds, ax			; with correct properties
	mov	es, ax
	mov	ss, ax
	mov	al, 0
	mov	fs, ax
	mov	gs, ax
	mov	eax, cr0		; turn off paging (CR0.PG) */
	and	eax, 0x7fffffff
	mov	cr0, eax
	mov	ecx, 0xc0000080		; turn off long mode support (EFER.LME)
	rdmsr
	and	ah, 0b11111110
	wrmsr
	mov	eax, cr4		; turn off paging extensions
	and	al, 0b11001111		; (CR4.PAE, CR4.PSE)
	mov	cr4, eax
	mov	eax, cr3		; invalidate TLB by touching CR3
	mov	cr3, eax
	pop	eax			; set base mem. size in BIOS data area
	mov	[word 0x0413], ax
	pop	eax			; also set EBDA segment
	mov	[word 0x040e], ax
	mov	dx, 0x03cc		; frob VGA miscellaneous output reg.
	in	al, dx			; --- to make sure CRTC appears at
	or	al, 0b00000001		; ports 0x03d4 etc. not 0x03b4 etc.
	mov	dl, 0xc2
	out	dx, al
	lea	ebp, [esp+4]		; point ebp to the boot parameters
	ret				; jump to stage 2 entry point

	align	2
Lpm32_gdtr:
	dw	Lpm32_gdt_end-Lpm32_gdt-1
	dq	Lpm32_gdt-LB
	align	8
Lpm32_gdt equ	$-8
	dq	0x00cf9a000000ffff	; 32-bit protected mode code seg.
	dq	0x00cf92000000ffff	; 32-bit protected mode data seg.
Lpm32_gdt_end:

LE:					; <<< end of trampoline to copy >>>
					; must be 8-byte aligned w.r.t. LB
