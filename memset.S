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
 * Moderately fast implementation of memset(...) & some other memory-filling
 * functions.
 *
 * Since GNU EFI also defines a memset(...) --- a less efficient
 * implementation --- we export this implementation as a wrapper function
 * which can then be activated with the --wrap linker flag.
 */

	.globl	__wrap_memset
	.weak	memset
	.globl	_memset_pat2
	.globl	_memset_pat3
	.globl	_memset_pat4
__wrap_memset:
memset:
	movb	%dl, %dh
_memset_pat2:
	movzwl	%dx, %edx
	imull	$0x10001, %edx, %edx
_memset_pat4:
	movl	%edx, %edx		/* spread the 32-bit pattern over */
	movl	%edx, %eax		/* 64 bits */
	shlq	$32, %rax
	orq	%rdx, %rax
	pushq	%rdi			/* preserve %rdi */
	pushq	%rcx			/* remember the return value */
	movq	%rcx, %rdi		/* load registers for a set */
	movq	%r8, %rcx
	shrq	$3, %rcx		/* set quadwords */
	rep stosq
	movl	%r8d, %ecx		/* set bytes */
	andl	$7, %ecx
	jecxz	.Ldone
.Lset_bytes:
	stosb
	rorq	$8, %rax
	loop	.Lset_bytes
.Ldone:
	popq	%rax
	popq	%rdi
	retq

	.globl	_memset_pat3
_memset_pat3:
	pushq	%rdi			/* preserve %rdi */
	pushq	%rcx			/* remember the return value */
	movq	%rcx, %rdi		/* load registers for a set */
	movq	%r8, %rcx
	andl	$0xffffff, %edx		/* spread the 24-bit pattern over */
	movl	%edx, %eax		/* 64 bits */
	shlq	$24, %rax
	orq	%rdx, %rax
	shlq	$24, %rax
	orq	%rdx, %rax
	shrq	$3, %rcx		/* set quadwords */
	jrcxz	.Ldone_quadwords_pat3
.Lset_quadwords_pat3:
	stosq
	rorq	$16, %rax
	loop	.Lset_quadwords_pat3
.Ldone_quadwords_pat3:
	movl	%r8d, %ecx		/* set bytes */
	jecxz	.Ldone_pat3
.Lset_bytes_pat3:
	stosb
	rorq	$8, %rax
	loop	.Lset_bytes_pat3
.Ldone_pat3:
	popq	%rax
	popq	%rdi
	retq
