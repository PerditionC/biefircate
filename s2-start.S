/*
 * Copyright (c) 2021 TK Chia
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of the developer(s) nor the names of its
 *     contributors may be used to endorse or promote products derived from
 *     this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

	.section .text, "a"

	.globl	_start
_start:
	movl	$0x0400, %esp
	movzwl	0x0413, %edi
	shll	$10, %edi
	subl	$vga_init_end-vga_init_start, %edi
	movl	%edi, %eax
	or	%eax, gdt16+8+2
	movl	$vga_init_start, %esi
	movl	$(vga_init_end-vga_init_start)/4, %ecx
	cld
	rep movsd
	shrl	$4, %eax
	movw	%ax, vga_init_seg-vga_init_end(%edi)
	lgdt	gdtr16
	lidt	idtrrm16
	ljmpw	$8, $trampoline-vga_init_start

	.section .rodata, "a"

	.balign	8
gdt16 = . - 8
	.quad	0x008f9a000000ffff	/* 16-bit protected mode code seg. */
	.quad	0x008f92000000ffff	/* 16-bit protected data code seg. */
gdt16_end:
gdtr16:	.hword	gdt16_end-gdt16-1
	.long	gdt16
idtrrm16:
	.hword	0x100*4-1
	.long	0

	.balign	16
	.code16
vga_init_start:
	/*
	 * Include the SeaVGABIOS code here.  This will be copied to base
	 * memory.
	 *
	 * NOTE: the SeaVGABIOS code is released under the GNU LGPL v3.
	 */
	.incbin	"/usr/share/seabios/vgabios-stdvga.bin"

	.balign	16
trampoline:
	movw	$0x10, %ax		/* prime segment descriptor caches */
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %ss
	movw	%ax, %fs
	movw	%ax, %gs
	movl	%cr0, %eax		/* switch to real mode */
	andb	$~0x01, %al
	movl	%eax, %cr0
	ljmpw	$0, $cont-vga_init_start
vga_init_seg = . - 2
cont:
	xorw	%ax, %ax		/* really set up segments */
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %ss
	movw	%ax, %fs
	movw	%ax, %gs
	pushw	%cs			/* call SeaVGABIOS */
	callw	vga_init_start+3
	movw	$0x0003, %ax		/* try to set 80 * 25 screen mode */
	int	$0x10
	hlt
	.balign	16
vga_init_end:
