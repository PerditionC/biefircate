/*
 * Copyright (c) 2020 TK Chia
 *
 * This file is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */

/*
 * Re-setup some stuff after exiting boot services.  Specifically, re-setup
 * long mode itself to use our own GDT, IDT, & page tables which reside below
 * the 4 GiB mark.  Disable interrupts.
 *
 * Also set the address to use for the long mode <-> real mode trampolines, &
 * set up the necessary code & data structures there.  The address should be
 * below the 1 MiB mark.
 */
	.globl	lm86_rm86_init
lm86_rm86_init:
	pushq	%rsi
	pushq	%rdi
	movw	$.Lgo_to_lm-.LB, 4*0x20	/* install an int 0x20 handler */
	movw	%cx, 4*0x20+2
	movw	%cx, .Lrm_t_seg(%rip)	/* record the segment */
	movzwq	%cx, %rdi		/* record the absolute address */
	shlq	$4, %rdi
	movl	%edi, .Lrm_t_addr(%rip)
	movl	%edi, %eax
	leaq	.LB(%rip), %rsi		/* copy the real mode trampolines */
	movq	$(.LE-.LB)/8,%rcx	/* to base memory */
	rep movsq
					/* fix up everything */
	addl	%eax, .Lour_gdtr+2-.LE(%rdi)
	addl	%eax, .Lour_gdt+8+2-.LE(%rdi)
	addl	%eax, .Lour_gdt+0x10+2-.LE(%rdi)
	addl	%eax, .Llm_ip-.LE(%rdi)
	cli				/* we must disable interrupts now */
	movq	%cr0, %rax		/* save our CR0 & CR4 (for use when */
	movl	%eax, .Llm_cr0-.LE(%rdi)/* returning from real mode) */
	movq	%cr4, %rax
	movl	%eax, .Llm_cr4-.LE(%rdi)
	movq	%cr3, %rax		/* save CR3 (FIXME) */
	movl	%eax, .Llm_cr3-.LE(%rdi)
	lgdt	.Lour_gdtr-.LE(%rdi)	/* switch to our GDT, %cs, etc. */
	pushq	$SEL_LM_TEXT
	leaq	.Linit_cont(%rip), %rax
	pushq	%rax
	lretq
.Linit_cont:
	movw	$SEL_LM_DATA, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %ss
	movw	%ax, %fs
	movw	%ax, %gs
	popq	%rdi
	popq	%rsi
	retq

/*
 * Return the address of the rm86_regs_t structure containing register
 * values for real mode operation.  This should be called after
 * rm86_set_trampolines_seg(.).
 */
	.globl	rm86_regs
rm86_regs:
	movl	.Lrm_t_addr(%rip), %eax
	add	$.Lrm_regs-.LB, %rax
	retq

/*
 * Run some code in real mode.  On return, the register values at
 * *rm86_regs() will be updated.
 */
	.globl	rm86
rm86:
	pushq	%rbx			/* save all call-saved registers */
	pushq	%rbp
	pushq	%rsi
	pushq	%rdi
	pushq	%r12
	pushq	%r13
	pushq	%r14
	pushq	%r15
	pushfq				/* save flags */
	movq	%rsp, %rax
	cli
	movl	.Lrm_t_addr(%rip),%esp	/* save %rsp */
	movq	%rax, .Llm_rsp-.LB(%rsp)
	movq	%cr3, %rcx
	movq	%rcx, .Llm_cr3-.LB(%rsp)
	movq	(%rcx), %rdx		/* also quickly stash the first */
	movq	%rdx, .Lpml4-.LB(%rsp)	/* Page-Map Level 4 (PML4) table
					   entry, so that we can switch to
					   a temporary page table in base
					   memory before returning to the
					   real page table */
	lgdt	.Lour_gdtr-.LB(%rsp)	/* go to 16-bit protected mode */
	pushq	$8
	pushq	$.Lgo_to_rm-.LB
	lretq

	.balign	8
.LB = . - 8				/* start of area to copy to base mem */

.Lpml4 = . - 8				/* space for temporary PML4 table
					/* entry; must be 4 KiB-aligned
					 * when copied out */
	.code16
.Lgo_to_rm:
	movw	$0x10, %cx		/* prime segment descriptor caches */
	movw	%cx, %ds		/* with correct properties */
	movw	%cx, %es
	movw	%cx, %ss
	movw	%cx, %fs
	movw	%cx, %gs
	data32 lidt .Lrm_idtr-.LB	/* load real-mode IDT */
	andl	$0x7ffffffe, %eax	/* turn off protected mode (CR0.PE) */
	movl	%eax, %cr0		/* & paging (CR0.PG) --- we are now
					   in real mode */
	movl	$0xc0000080, %ecx	/* turn off LM support (EFER.LME) */
	rdmsr
	and	$0xfe, %ah
	wrmsr
	andb	$0xcf, %bl		/* turn off paging extensions */
	movl	%ebx, %cr4		/* (CR4.PAE, CR4.PSE) */
	movw	$.Lrm_regs-.LE, %sp	/* load registers & hand over to */
	popal				/* whatever real mode code we want */
	popw	%es			/* to run */
	popw	%ds
	popw	%fs
	popw	%gs
	lss	%cs:.Lrm_regs+44-.LB, %esp
	pushl	%cs:.Lrm_regs+40-.LB
	popfl
	ljmpw	*%cs:.Lrm_regs+50-.LB

.Lgo_to_lm:				/* assume we are here from an `int' */
	popl	%cs:.Lrm_regs+50-.LB	/* store %ip, %cs */
	popfw				/* store flags */
	pushfl
	popl	%cs:.Lrm_regs+40-.LB
	cli
	lgdt	%cs:.Lour_gdtr-.LB	/* load our GDT */
					/* store %esp, %ss */
	movl	%esp, %cs:.Lrm_regs+44-.LB
	movw	%ss, %cs:.Lrm_regs+48-.LB
	movw	%cs, %sp		/* store the other registers */
	movw	%sp, %ss
	movw	$.Lrm_regs+40-.LB, %sp
	pushw	%gs
	pushw	%fs
	pushw	%ds
	pushw	%es
	pushal
	movl	$0xaaaaaaaa, %eax	/* restore CR3 (FIXME) */
.Llm_cr3 = . - 4
	movl	%eax, %cr3
	wbinvd				/* FIXME? */
	movl	$0xaaaaaaaa, %eax	/* restore CR4 */
.Llm_cr4 = . - 4
	movl	%eax, %cr4
	movl	$0xc0000080, %ecx	/* turn on LM support (EFER.LME) */
	rdmsr
	or	$0x01, %ah
	wrmsr
	movl	$0xaaaaaaaa, %eax	/* restore CR0 --- enter long mode */
.Llm_cr0 = . - 4
	movl	%eax, %cr0
	.byte	0x66, 0xea		/* "can't handle non absolute */
	.int	.Lback_in_lm-.LB	/* segment in `ljmp'" :-( */
	.hword	SEL_LM_TEXT
.Llm_ip = . - 6

	.code64
.Lback_in_lm:				/* restore %rsp */
	movabsq $0xaaaaaaaaaaaaaaaa, %rsp
.Llm_rsp = . - 8
	movw	$SEL_LM_DATA, %bx
	movw	%bx, %ds
	movw	%bx, %es
	movw	%bx, %ss
	movw	%bx, %fs
	movw	%bx, %gs
	/* sti */
	popfq				/* restore flags */
	popq	%r15			/* restore call-saved registers */
	popq	%r14
	popq	%r13
	popq	%r12
	popq	%rdi
	popq	%rsi
	popq	%rbp
	popq	%rbx
	retq

	.balign 2
.Lour_gdtr:
	.hword	.Lour_gdt_end-.Lour_gdt-1
	.quad	.Lour_gdt-.LB
.Lrm_idtr:
	.hword	4*0x100-1
	.long	0
	.balign	8
.Lour_gdt = . - 8
SEL_PM16_TEXT = . - .Lour_gdt
	.quad	0x008f9a000000ffff	/* 16-bit protected mode code seg. */
SEL_PM16_DATA = . - .Lour_gdt
	.quad	0x008f92000000ffff	/* 16-bit protected mode data seg. */
SEL_LM_TEXT = . - .Lour_gdt
	.quad	0x00af9a0000000000	/* our long mode code segment */
SEL_LM_DATA = . - .Lour_gdt
	.quad	0x00af920000000000	/* our long mode data segment */
.Lour_gdt_end:

	.balign	8
.LE:					/* end of area to copy to base mem.;
					   this _must_ be 8-byte aligned */
.Lrm_regs = .

	.lcomm	.Lrm_t_addr, 4
	.lcomm	.Lrm_t_seg, 2
