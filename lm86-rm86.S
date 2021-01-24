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

#include "truckload.h"

	.section .text.i, "x"

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
	movl	%ecx, %eax		/* record the absolute address */
	movl	%eax, %edi
	movl	%edi, .Lrm_t_addr(%rip)
	shrl	$4, %ecx
	movw	$.Lgo_to_lm-.LB, 4*0x20	/* install an int 0x20 handler */
	movw	%cx, 4*0x20+2
	movw	%cx, .Lrm_t_seg(%rip)	/* record the segment of the
					   trampolines */
	leaq	.LB(%rip), %rsi		/* copy the real mode trampolines */
	movq	$(.LE-.LB)/8, %rcx	/* to base memory */
	rep movsq
					/* fix up everything */
	addl	%eax, .Lour_gdtr+2-.LE(%rdi)
	addl	%eax, .Lour_gdt+SEL_PM16_TEXT+2-.LE(%rdi)
	addl	%eax, .Lour_gdt+SEL_PM16_DATA+2-.LE(%rdi)
	addl	%eax, .Llm_ip-.LE(%rdi)
	leaq	.Lour_tss(%rip), %rax
					/* fill up the TSS while at it */
	leaq	.Lint_stack_1_end(%rip), %rcx
	movq	%rcx, 0x4(%rax)		/* RSP0 */
	movq	%rcx, 0xc(%rax)		/* RSP1 */
	movq	%rcx, 0x14(%rax)	/* RSP2 */
	movq	%rcx, 0x24(%rax)	/* IST1 */
	movq	%rcx, 0x3c(%rax)	/* IST4 */
	movq	%rcx, 0x44(%rax)	/* IST5 */
	movq	%rcx, 0x4c(%rax)	/* IST6 */
	movq	%rcx, 0x54(%rax)	/* IST7 */
	leaq	.Lint_stack_2_end(%rip), %rcx
	movq	%rcx, 0x2c(%rax)	/* IST2 */
	leaq	.Lint_stack_3_end(%rip), %rcx
	movq	%rcx, 0x34(%rax)	/* IST3 */
	movw	$TSS_IO_PERMS_OFF, 0x66(%rax)
					/* continue fixing up everything */
	movw	%ax, .Lour_gdt+SEL_TSS+2-.LE(%rdi)
	shrq	$16, %rax
	movb	%al, .Lour_gdt+SEL_TSS+4-.LE(%rdi)
	movb	%ah, .Lour_gdt+SEL_TSS+7-.LE(%rdi)
	shrq	$16, %rax
	movl	%eax, .Lour_gdt+SEL_TSS+8-.LE(%rdi)
	movq	%cr0, %rax		/* save our CR0 & CR4 (for use when */
	movl	%eax, .Llm_cr0-.LE(%rdi)/* returning from real mode) */
	movq	%cr4, %rax
	movl	%eax, .Llm_cr4-.LE(%rdi)
	movq	%rdx, %cr3		/* switch to new CR3; save it too */
	wbinvd
	movl	%edx, .Llm_cr3-.LE(%rdi)
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
	lidt	.Lour_idtr-.LE(%rdi)	/* switch to our IDT */
	movb	$SEL_TSS, %al		/* switch to our TSS */
	ltr	%ax
	leaq	.Lour_idt(%rip), %rdi	/* fill up the IDT */
	leaq	.Lisr_shims(%rip), %rdx
	movl	$0x100, %ecx
.Lfill_idt:
	movl	$SEL_LM_TEXT<<16, %eax
	movw	%dx, %ax
	stosl
	movl	%edx, %eax
	movw	$0x8e00 | INT_STACK_EXCP, %ax
	stosl
	movq	%rdx, %rax
	shrq	$32, %rax
	stosq
	addq	$ISR_SHIM_LEN, %rdx
	loop	.Lfill_idt
					/* have the NMI & double fault ISRs
					   use a different stack */
	movb	$INT_STACK_BAD_EXCP, .Lour_idt+2*0x10+4(%rip)
	movb	$INT_STACK_BAD_EXCP, .Lour_idt+8*0x10+4(%rip)
	leaq	.Lisr_shims(%rip), %rdi	/* fill in the shims for the ISRs */
	movb	$0, %al			/* & set them to call the default
					   handler which panics */
	leaq	.Ldefault_isr(%rip), %rdx
	leaq	.Lisr_shim_epilog(%rip), %r8
.Lfill_isr_shims:
	leaq	.LBI(%rip), %rsi
	movq	$ISR_SHIM_LEN/8, %rcx
	rep movsq
	movb	%al, .Lint_num-.LEI(%rdi)
	movq	%rdx, .Lisr_addr-.LEI(%rdi)
	movq	%r8, .Lisr_shim_epilog_addr-.LEI(%rdi)
	incb	%al
	jnz	.Lfill_isr_shims
	popq	%rdi
	popq	%rsi
	retq

	.text

.Ldefault_isr:
	movq	%cr2, %rax		/* read CR2 & push value on stack */
	pushq	%rax
	pushq	%r8			/* push error code on stack */
	leaq	.Ldefault_isr_msg(%rip), %r8
	subq	$0x20, %rsp
	call	panic_with_far_caller

/* Set the interrupt handling routine for a particular interrupt vector. */
	.globl	lm86_set_isr
lm86_set_isr:
	movzbq	%cl, %rcx
	imulq	$ISR_SHIM_LEN, %rcx, %rcx
	movq	%rdx, .Lisr_addr-.LBI(%rcx)
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

	.section .text.i, "x"

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
	movl	$0xaaaaaaaa, %eax	/* restore CR3 if it was changed */
.Llm_cr3 = . - 4			/* (FIXME?  only checks low 32 bits) */
	movl	%cr3, %edx
	cmpl	%eax, %edx
	jz	.Lcr3_right
	movl	%eax, %cr3
	wbinvd
.Lcr3_right:
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
.Lback_in_lm:
					/* restore %rsp */
	movabsq $0xaaaaaaaaaaaaaaaa, %rsp
.Llm_rsp = . - 8
	movw	$SEL_LM_DATA, %bx	/* restore segment registers */
	movw	%bx, %ds
	movw	%bx, %es
	movw	%bx, %ss
	movw	%bx, %fs
	movw	%bx, %gs
	lidt	.Lour_idtr(%rip)	/* load our IDT & TSS */
	movb	$SEL_TSS, %bl
	ltr	%bx
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
.Lour_idtr:
	.hword	0x100*0x10-1
	.quad	.Lour_idt
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
SEL_TSS = . - .Lour_gdt			/* our TSS */
	.hword	TSS_SIZE-1, 0
	.long	0x00008900
	.quad	0x0000000000000000
.Lour_gdt_end:

	.balign	8
.LE:					/* end of area to copy to base mem.;
					   this _must_ be 8-byte aligned */
.Lrm_regs = .

	.balign	8
.LBI = .				/* start of template for interrupt
					   handler shims */
	testb	$8, %spl		/* if %rsp is not 16-byte aligned, */
	jz	.Lhave_err_code		/* then it means no error code was */
	pushq	$-1			/* pushed; push a dummy code */
.Lhave_err_code:
	pushq	%rax
	pushq	%rcx
	pushq	%rdx
	pushq	%r8
	pushq	%r9
	pushq	%r10
	pushq	%r11
	movw	%ds, %ax
	shlq	$16, %rax
	movw	%es, %ax
	shlq	$16, %rax
	movw	%fs, %ax
	shlq	$16, %rax
	movw	%gs, %ax
	pushq	%rax
	movw	$SEL_LM_DATA, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %ss
	movw	%ax, %fs
	movw	%ax, %gs
	movzwq	0x50(%rsp), %rcx
	movq	0x48(%rsp), %rdx
	movq	0x40(%rsp), %r8
	xorq	%r9, %r9
	movb	$0xaa, %r9b
.Lint_num = . - 1
	subq	$0x20, %rsp
	movabsq	$0xaaaaaaaaaaaaaaaa, %rax
.Lisr_shim_epilog_addr = . - 8
	pushq	%rax
	movabsq	$0xaaaaaaaaaaaaaaaa, %rax
.Lisr_addr = . - 8
	jmpq	*%rax
ISR_SHIM_LEN = 0x70
	.org	.LBI+ISR_SHIM_LEN
.LEI = .LBI+ISR_SHIM_LEN		/* end of template for interrupt
					   handler shims */

	.text

.Lisr_shim_epilog:
	addq	$0x20, %rsp
	popq	%rax
	movw	%ax, %gs
	shrq	$16, %rax
	movw	%ax, %fs
	shrq	$16, %rax
	movw	%ax, %es
	shrq	$16, %rax
	movw	%ax, %ds
	popq	%r11
	popq	%r10
	popq	%r9
	popq	%r8
	popq	%rdx
	popq	%rcx
	popq	%rax
	addq	$8, %rsp
	iretq

	.section .rdata, "dr"

.Ldefault_isr_msg:
	.ascii	"stray interrupt 0x%02x\n"
	.asciz	"  error code: %#llx  CR2: %#llx"

	.lcomm	.Lrm_t_addr, 4
	.lcomm	.Lrm_t_seg, 2
	.lcomm	.Lisr_shims, 0x100*(.LEI-.LBI)
	.lcomm	.Lour_idt, 0x100*0x10, 8
TSS_IO_PERMS_OFF = 0x68
TSS_SIZE = TSS_IO_PERMS_OFF
	.lcomm	.Lour_tss, TSS_SIZE, 4

INT_STACK_1_SIZE = 0x0c00
INT_STACK_2_SIZE = 0x0400
INT_STACK_3_SIZE = 0x1000
INT_STACKS_SIZE = INT_STACK_1_SIZE+INT_STACK_2_SIZE+INT_STACK_3_SIZE
	/*
	 * We really want the stacks to be 16-bit aligned:  the CPU will
	 * round down the ISTn addresses in the TSS to multiples of 16
	 * before using them.
	 *
	 * Alas the UEFI loader in OVMF (at least) seems to insist on loading
	 * our program only at an 8-byte boundary, no matter what the section
	 * headers say.
	 *
	 * So just ask the loader to place the stacks at an 8-byte boundary.
	 */
	.lcomm	.Lint_stacks, INT_STACKS_SIZE, 8
.Lint_stack_1_end = .Lint_stacks+INT_STACK_1_SIZE
.Lint_stack_2_end = .Lint_stack_1_end+INT_STACK_2_SIZE
.Lint_stack_3_end = .Lint_stack_2_end+INT_STACK_3_SIZE
.Lint_stacks_end = .Lint_stack_3_end
