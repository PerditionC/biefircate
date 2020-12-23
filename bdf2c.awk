#!/usr/bin/awk -f
# Copyright (c) 2020 TK Chia
#
# This file is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 2 of the License, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.

# This is a simplistic program to convert a .bdf font file for an 8 * x font
# into a C module source file (.c) or a C header file (.h).  The output goes
# to stdout.
#
# Usage:
#
#	bdf2c [H=1] [PUA=0] [SP=0] [N=(font-name)] \
#	      [(in.bdf)] [> {(out.c) | (out.h)}]
#
# Options:
#	H=1		output a header file, not a C module file
#	PUA=0		exclude Unicode Private Use Areas
#	SP=0		exclude Unicode supplementary planes
#	N=(font-name)	set font name

function error(msg)
{
	err_msg = "line " NR ": " msg
	exit 1
}

function mergesort(ary, lo, hi, \
		   mid, i, j, k, tmp)
{
	if (lo >= hi)
		return
	mid = int((lo + hi) / 2)
	mergesort(ary, lo, mid)
	mergesort(ary, mid + 1, hi)
	i = lo
	j = lo
	k = mid + 1
	while (i <= hi) {
		if (j <= mid && k <= hi) {
			if (ary[j] < ary[k]) {
				tmp[i] = ary[j]
				j += 1
			} else {
				tmp[i] = ary[k]
				k += 1
			}
		} else if (j <= mid) {
			tmp[i] = ary[j]
			j += 1
		} else {
			tmp[i] = ary[k]
			k += 1
		}
		i += 1
	}
	for (i = lo; i <= hi; i += 1)
		ary[i] = tmp[i]
}

function new_char()
{
	curr_height = 0
	rows_left = 0
	curr_code = 0
	curr_bitmap = ""
}

BEGIN {
	H += 0
	if (PUA == "")
		PUA = 1
	PUA += 0
	if (SP == "")
		SP = 1
	SP += 0
	err_msg = ""
	n_codes = 0
	max_height = 0
	new_char()
}

/^[ \t]*[0123456789abcdefABCDEF]+[ \t]*$/ {
	if (rows_left) {
		rows_left -= 1
		if (length($1) != 2)
			error("bitmap too wide or too narrow")
		curr_bitmap = curr_bitmap "\t\t0x" tolower($1) ",\n"
		next
	}
}

/^[ \t]*ENCODING[ \t]+/ {
	curr_code = $2 + 0
	if (curr_code <= 0)
		error("bad code point " curr_code)
	next
}

/^[ \t]*BBX[ \t]+/ {
	curr_height = $3 + 0
	if (curr_height == 0)
		error("bitmap height bogus")
	if (curr_height > max_height)
		max_height = curr_height
	next
}

/^[ \t]*BITMAP[ \t]*$/ {
	if (curr_height == 0)
		error("bitmap height undefined")
	rows_left = curr_height
	next
}

/^[ \t]*ENDCHAR[ \t]*$/ {
	if (curr_code == 0)
		error("code point undefined")
	if (curr_bitmap == "")
		error("bitmap undefined")
	if (!SP && curr_code > 0xffff)
		next
	if (!PUA && ((curr_code >= 0x00e000 && curr_code <= 0x00f8ff) ||
		     (curr_code >= 0x0f0000 && curr_code <= 0x0ffffd) ||
		     (curr_code >= 0x100000 && curr_code <= 0x10fffd)))
		next
	if (code_seen[curr_code])
		error("code point " curr_code " appears more than once")
	n_codes += 1
	codes[n_codes] = curr_code
	code_seen[curr_code] = 1
	bitmap[curr_code] = curr_bitmap
	new_char()
	next
}

/^[ \t]*ENDFONT[ \t]*$/ {
	exit
}

END {
	if (err_msg == "" && n_codes == 0)
		err_msg = "empty font"
	if (err_msg != "") {
		print "error: " err_msg >"/dev/stderr"
		exit 1
	}
	mergesort(codes, 1, n_codes)
	if (N == "")
		N = "default"
	print "/****** AUTOMATICALLY GENERATED ******/"
	if (H) {
		print "#ifndef H_FONT_" N
		print "#define H_FONT_" N
		print "#include <inttypes.h>"
		print "#include <wchar.h>"
		print "#define FONT_" toupper(N) "_GLYPHS " n_codes
		print "#define FONT_" toupper(N) "_WIDTH 8"
		print "#define FONT_" toupper(N) "_HEIGHT " max_height
		print "extern const wchar_t " \
			  "font_" N "_code_points[" n_codes "];"
		print "extern const uint8_t " \
			  "font_" N "_data[" n_codes "][" max_height "];"
		print "#endif"
	} else {
		print "#include <inttypes.h>"
		print "#include <wchar.h>"
		print "const wchar_t font_" N "_code_points[" n_codes "] = {"
		for (i = 1; i <= n_codes; i += 1)
			print "\t" codes[i] ","
		print "};\n"
		print "const uint8_t font_" N "_data[" n_codes \
						       "][" max_height "] = {"
		for (i = 1; i <= n_codes; i += 1) {
			curr_code = codes[i]
			print "\t{"
			print bitmap[curr_code], "\t},"
		}
		print "};\n"
	}
}
