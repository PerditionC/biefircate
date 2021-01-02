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

/* Naive implementation of strcmp(, ) & strncmp(, , ). */

	.globl	strcmp
	.globl	strncmp
strcmp:
	orq	$-1, %r8
strncmp:
	pushq	%rsi			/* preserve %rsi & %rdi */
	pushq	%rdi
	movq	%rcx, %rsi		/* load registers for a comparison */
	movq	%rdx, %rdi
	movq	%r8, %rcx
	jrcxz	1f			/* if buffer size is 0, bail out */
0:					/* compare until there is a */
	lodsb				/* difference, or there is a null */
	scasb				/* byte, or the buffer size runs */
	jnz	8f			/* out, whichever comes first */
	testb	%al, %al
	loopnz	0b
1:
	xorl	%eax, %eax		/* strings are the same up to the */
	popq	%rdi			/* null byte or up to the buffer */
	popq	%rsi			/* size end; return 0 */
	retq
8:
	sbbl	%eax, %eax		/* CF = 1 => we should return < 0 */
	shll	%eax			/* CF = 0 => we should return > 0 */
	incl	%eax
	popq	%rdi			/* restore %rsi & %rdi */
	popq	%rsi
	retq
