/*
 * Copyright (c) 2020--2021 TK Chia
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

	.globl	run_stage2
run_stage2:
	cli
	leal	0x1000(%edx), %esp	/* switch to new stack */
	xorq	%rax, %rax		/* push some dummy parameters to */
	pushq	%rax			/* keep the FreeBSD kernel happy (?) */
	pushq	%rax
	pushq	%rcx			/* push ELF32 entry point */
	pushq	%r8			/* push base mem. size (in KiB) */
	movl	%edx, %edi
	pushq	%rdi			/* push trampoline address */
	leaq	.LB(%rip), %rsi		/* copy trampoline to mem. < 4 GiB */
	movq	$(.LE-.LB)/8, %rcx
	rep movsq
	addl	%edx, .Lpm32_gdtr+2-.LE(%rdi) /* fix up GDTR value */
	retq				/* jump to trampoline */

	.balign	8
.LB = .					/* start of trampoline to copy */

	lgdt	.Lpm32_gdtr-.LE(%rdi)	/* go to 32-bit protected mode */
	pushq	$8
	leaq	.Lpm32-.LE(%rdi), %rdi
	pushq	%rdi
	lretq

	.code32

.Lpm32:
	movw	$0x10, %ax		/* prime segment descriptor caches */
	movw	%ax, %ds		/* with correct properties */
	movw	%ax, %es
	movw	%ax, %ss
	xorb	%al, %al
	movw	%ax, %fs
	movw	%ax, %gs
	movl	%cr0, %eax		/* turn off paging (CR0.PG) */
	andl	$0x7fffffff, %eax
	movl	%eax, %cr0
	movl	$0xc0000080, %ecx	/* turn off LM support (EFER.LME) */
	rdmsr
	and	$0xfe, %ah
	wrmsr
	movl	%cr4, %ebx		/* turn off paging extensions */
	andb	$0xcf, %bl		/* (CR4.PAE, CR4.PSE) */
	movl	%ebx, %cr4
	wbinvd
	popl	%eax			/* set base mem. size in */
	movw	%ax, 0x0413		/* BIOS data area */
	popl	%ebx
	movw	$0x03cc, %dx		/* frob VGA miscellaneous output */
	inb	%dx, %al		/* reg. --- make sure CRTC */
	orb	$0x01, %al		/* appears at ports 0x03d4 etc., */
	movb	$0xc2, %dl		/* not 0x03b4 etc. */
	outb	%al, %dx
	retl				/* jump to the stage2 entry point */

	.balign 2
.Lpm32_gdtr:
	.hword	.Lpm32_gdt_end-.Lpm32_gdt-1
	.quad	.Lpm32_gdt-.LB
	.balign	8
.Lpm32_gdt = . - 8
	.quad	0x00cf9a000000ffff	/* 32-bit protected mode code seg. */
	.quad	0x00cf92000000ffff	/* 32-bit protected mode data seg. */
.Lpm32_gdt_end:

.LE:					/* end of trampoline to copy; must
					   be 8-byte aligned w.r.t. .LB */
