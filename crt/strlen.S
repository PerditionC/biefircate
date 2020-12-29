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

/* Naive implementation of strlen(.). */

	.globl	strlen
strlen:
	pushq	%rdi			/* preserve %rdi */
	movq	%rcx, %rdi		/* load registers for a scan */
	stc
	sbbq	%rcx, %rcx
	xorb	%al, %al
	repnz scasb			/* scan */
	leaq	2(%rcx), %rax		/* compute string length from */
	negq	%rax			/* resulting %rcx */
	popq	%rdi			/* restore %rdi */
	retq
