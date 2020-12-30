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

/* Naive implementation of strncpy(, , ). */

	.globl	strncpy
strncpy:
	pushq	%rsi			/* preserve %rsi & %rdi */
	pushq	%rdi
	movq	%rcx, %rdi		/* load registers for a copy */
	movq	%rdx, %rsi
	movq	%rcx, %rdx		/* remember the destination pointer */
	movq	%r8, %rcx
	jrcxz	9f			/* if buffer length is 0, bail out
					   immediately */
0:					/* copy source string until we hit */
	lodsb				/* a null byte, or until the length */
	stosb				/* is exhausted, whichever comes */
	testb	%al, %al		/* first */
	loopnz	0b
	xorb	%al, %al		/* fill the rest of the buffer */
	rep stosb			/* with nulls */
9:
	xchgq	%rdx, %rax		/* return the destination pointer */
	popq	%rdi			/* restore %rsi & %rdi */
	popq	%rsi
	retq
