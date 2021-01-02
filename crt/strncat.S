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

/* Naive implementation of strcat(, ). */

	.globl	strcat
	.globl	strncat
strcat:
	orq	$-1, %r8
strncat:
	pushq	%rsi			/* preserve %rsi & %rdi */
	pushq	%rdi
	pushq	%rcx
	movq	%rcx, %rdi		/* load registers */
	movq	%rdx, %rsi
	orq	$-1, %rcx		/* scan for end of dest. string */
	xorb	%al, %al
	repnz scasb
	decq	%rdi
	movq	%r8, %rcx
0:					/* copy */
	lodsb
	stosb
	testb	%al, %al
	loopnz	0b
	jz	1f
	mov	%cl, (%rdi)		/* make sure dest. has null byte */
1:
	popq	%rax			/* return the destination pointer */
	popq	%rdi			/* restore %rsi & %rdi */
	popq	%rsi
	retq
