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

/* Switch to our own stack memory before starting efi_main(, ). */
	.globl	_start
_start:
	leaq	.Lstack_begin(%rip), %rsp
	jmp	efi_main

	.bss
	.skip	0x10000
.Lstack_begin:
