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

SECTIONS
{
	/* Initialization text. */
	.text.i : ALIGN(__section_alignment__)
	{
		*(.text.i .text.i.*)
	}
}
INSERT AFTER .text;

SECTIONS
{
	/* Initialization data. */
	.data.i : ALIGN(__section_alignment__)
	{
		*(.data.i .data.i.*)
	}
}
INSERT AFTER .data;
