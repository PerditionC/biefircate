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

	global	memcpy
	global	memmove
memcpy:
memmove:
	push	esi
	push	edi
	mov	edi, eax
	mov	esi, edx
	cmp	edi, esi
	ja	.backward
	je	.done
	mov	edx, ecx
	shr	ecx, 2
	rep movsd
.finish:
	mov	cl, dl
	and	cl, byte 3
	rep movsb
	cld
.done:
	pop	edi
	pop	esi
	ret
.backward:
	lea	edi, [edi+ecx-4]
	lea	esi, [esi+ecx-4]
	mov	edx, ecx
	shr	ecx, 2
	std
	rep movsd
	add	edi, byte 3
	add	esi, byte 3
	jmp	short .finish
