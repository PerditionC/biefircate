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
 * Moderately fast implementation of memmove(...) (& memcpy(...)).
 *
 * Since GNU EFI also defines a memcpy(...) --- a less efficient
 * implementation --- we export this implementation as a wrapper function
 * which can then be activated with the --wrap linker flag.
 */

	.globl	memmove
	.globl	__wrap_memcpy
	.weak	memcpy
memmove:
__wrap_memcpy:
memcpy:
	pushq	%rsi			/* preserve %rsi & %rdi */
	pushq	%rdi
	pushq	%rcx			/* remember the return value */
	movq	%rcx, %rdi		/* load registers for a move */
	movq	%rdx, %rsi
	movq	%r8, %rcx
	jrcxz	.Ldone			/* if nothing to copy, bail out */
	cmpq	%rsi, %rdi		/* if dest == src, bail out */
	jz	.Ldone
	ja	.Lbackwards		/* if dest > src, we must copy
					   backwards */
	/* forward copy */
	movl	%ecx, %edx		/* copy quadwords */
	shrq	$3, %rcx
	rep movsq
	movl	%edx, %ecx		/* copy remaining bytes */
	andl	$7, %ecx
	rep movsb
.Ldone:
	popq	%rax
	popq	%rdi
	popq	%rsi
	retq
.Lbackwards:
	leaq	-1(%rdi,%rcx), %rdi	/* move %rsi & %rdi to respective */
	leaq	-1(%rsi,%rcx), %rsi	/* buffer ends */
	movq	%rcx, %rax		/* copy bytes */
	andl	$7, %ecx
	std
	rep movsb
	subq	$7, %rdi		/* adjust %rsi & %edi to copy */
	subq	$7, %rsi		/* quadwords */
	xchgq	%rax, %rcx		/* then copy quadwords */
	shrq	$3, %rcx
	rep movsq
	cld				/* reset the direction flag */
	jmp	.Ldone
