/*
 * Set the address to use for the long mode <-> real mode trampolines, & set
 * up the necessary code & data structures there.  The address should be below
 * the 1 MiB mark.
 */
	.globl	rm86_set_trampolines_seg
rm86_set_trampolines_seg:
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
	addl	%eax, .Lpm16_gdtr+2-.LE(%rdi)
	addl	%eax, .Lpm16_gdt+8+2-.LE(%rdi)
	addl	%eax, .Lpm16_gdt+0x10+2-.LE(%rdi)
	addl	%eax, .Llm_temp_cr3-.LE(%rdi)
	addl	%eax, .Llm_ip-.LE(%rdi)
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
	movw	%ds, %bx
	shlq	$16, %rbx
	movw	%es, %bx
	shlq	$16, %rbx
	movw	%ss, %bx
	pushq	%rbx
	movw	%fs, %bx
	shlq	$16, %rbx
	movw	%gs, %bx
	pushq	%rbx
	movq	%rsp, %rax
	cli
	movl	.Lrm_t_addr(%rip),%esp	/* save %rsp, GDTR, IDTR, CR0, CR3,
					   & CR4 */
	movq	%rax, .Llm_rsp-.LB(%rsp)
	sgdtq	.Llm_gdtr-.LB(%rsp)
	sidtq	.Llm_idtr-.LB(%rsp)
	movq	%cr0, %rax
	movl	%eax, .Llm_cr0-.LB(%rsp)
	movq	%cr3, %rcx
	movq	%rcx, .Llm_cr3-.LB(%rsp)
	movq	%cr4, %rbx
	movl	%ebx, .Llm_cr4-.LB(%rsp)
	movw	%cs, .Llm_cs-.LB(%rsp)	/* save our %cs */
	movq	(%rcx), %rdx		/* also quickly stash the first */
	movq	%rdx, .Lpml4-.LB(%rsp)	/* Page-Map Level 4 (PML4) table
					   entry, so that we can switch to
					   a temporary page table in base
					   memory before returning to the
					   real page table */
	lgdt	.Lpm16_gdtr-.LB(%rsp)	/* go to 16-bit protected mode */
	pushq	$8
	pushq	$.Lgo_to_rm-.LB
	lretq

	.balign	8
.LB = . - 8				/* start of area to copy to base mem */

	.code16

.Lpml4 = . - 8				/* space for temporary PML4 table
					/* entry; must be 4 KiB-aligned
					 * when copied out */

.Lgo_to_rm:
	movw	$0x10, %cx		/* prime segment descriptor caches */
	movw	%cx, %ds		/* with correct properties */
	movw	%cx, %es
	movw	%cx, %ss
	movw	%cx, %fs
	movw	%cx, %gs
	data32 lidt .Lrm_idtr-.LB	/* load real-mode IDTR */
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
	lgdt	%cs:.Lpm16_gdtr-.LB	/* temporarily load our GDTR which
					   is in base memory */
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
	movl	$.Lpml4-.LB, %eax	/* switch to a temporary page table */
.Llm_temp_cr3 = . - 4			/* in base memory space */
	movl	%eax, %cr3
	wbinvd				/* FIXME? */
	movl	$0xaaaaaaaa, %eax	/* restore CR4 */
.Llm_cr4 = . - 4
	movl	%eax, %cr4
	movl	$0xc0000080, %ecx	/* turn on LM support (EFER.LME) */
	rdmsr
	or	$0x01, %ah
	wrmsr
	movl	$0xaaaaaaaa, %eax	/* restore CR0 --- enter */
.Llm_cr0 = . - 4			/* protected mode */
	movl	%eax, %cr0
	ljmpl	$0x18, $.Lback_in_lm-.LB
.Llm_ip = . - 6

	.code64
.Lback_in_lm:
	lgdt	.Llm_gdtr(%rip)		/* restore actual GDTR & IDTR */
	lidt	.Llm_idtr(%rip)
					/* restore actual CR3 */
	movabsq	$0xaaaaaaaaaaaaaaaa, %rax
.Llm_cr3 = . - 8
	movq	%rax, %cr3
					/* restore %rsp */
	movabsq $0xaaaaaaaaaaaaaaaa, %rsp
.Llm_rsp = . - 8
	pushq	$0x0000aaaa		/* return to the original code seg. */
.Llm_cs = . - 4
	leaq	.Lback_in_lm.cont(%rip), %rax
	pushq	%rax
	lretq
.Lback_in_lm.cont:
	popq	%rbx			/* restore call-saved registers */
	movw	%bx, %gs
	shrq	$16, %rbx
	movw	%bx, %fs
	popq	%rbx
	movw	%bx, %ss
	shrq	$16, %rbx
	movw	%bx, %es
	shrq	$16, %rbx
	movw	%bx, %ds
	sti
	popq	%r15
	popq	%r14
	popq	%r13
	popq	%r12
	popq	%rdi
	popq	%rsi
	popq	%rbp
	popq	%rbx
	retq

	.balign 2
.Lpm16_gdtr:
	.hword	.Lpm16_gdt_end-.Lpm16_gdt-1
	.quad	.Lpm16_gdt-.LB
.Lrm_idtr:
	.hword	4*0x100-1
	.long	0
	.balign	8
.Lpm16_gdt = . - 8
	.quad	0x008f9a000000ffff	/* 16-bit protected mode code seg. */
	.quad	0x008f92000000ffff	/* 16-bit protected mode data seg. */
	.quad	0x00af9a0000000000	/* (temporary) long mode code seg. */
.Lpm16_gdt_end:

.LE:					/* end of area to copy to base mem. */

.Llm_gdtr = . + 8
.Llm_idtr = .Llm_gdtr + 10
.Lrm_regs = .Llm_idtr + 10

	.lcomm	.Lrm_t_addr, 4
	.lcomm	.Lrm_t_seg, 2
