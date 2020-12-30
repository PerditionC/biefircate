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

/* Naive implementation of strcpy(, ). */

	.globl	strcpy
strcpy:
	pushq	%rsi			/* preserve %rsi & %rdi */
	pushq	%rdi
	movq	%rcx, %rdi		/* load registers for a copy */
	movq	%rdx, %rsi
0:					/* copy */
	lodsb
	stosb
	testb	%al, %al
	jnz	0b
	xchgq	%rcx, %rax		/* return the destination pointer */
	popq	%rdi			/* restore %rsi & %rdi */
	popq	%rsi
	retq
