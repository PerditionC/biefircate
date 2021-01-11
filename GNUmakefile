# Copyright (c) 2020--2021 TK Chia
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

ifeq "" "$(wildcard config.cache)"
$(error you must configure this project first!)
endif

-include config.cache
-include $(conf_Lolwutconf_dir)/lolwutconf.mk

GNUEFISRCDIR = $(conf_Srcdir)/gnu-efi
ACPICASRCDIR = $(conf_Srcdir)/acpica
UNIVGASRCDIR = $(conf_Srcdir)/uni_vga
BDF2CSRCDIR = $(conf_Srcdir)/bdf2c-in-awk
#
# The MinGW toolchain defines the macro WIN32 & friends, & this confuses
# ACPICA.  Undefine the WIN32 macro.
#
# Also pre-define _CRTIMP to expand to nothing, so that <_mingw.h> does not
# try to put __attribute__((dllimport)) on <ctype.h> function declarations.
#
# And, predefine __USE_MINGW_ANSI_STDIO so that PRIx64 etc. are defined right.
#
CPPFLAGS = -I'$(abspath $(GNUEFISRCDIR))'/inc \
	   -I'$(abspath $(GNUEFISRCDIR))'/protocol \
	   -I'$(abspath $(GNUEFISRCDIR))'/inc/x86_64 \
	   -iquote '$(abspath $(ACPICASRCDIR))'/source/include \
	   -DGNU_EFI_USE_MS_ABI -UWIN32 -D_CRTIMP= -D__USE_MINGW_ANSI_STDIO
CFLAGS = -pie -fPIC -ffreestanding -Os -Wall -mno-red-zone \
    -fno-stack-protector -MMD
LDFLAGS = $(CFLAGS) -nostdlib -ffreestanding -Wl,--entry,_start \
	  -Wl,--subsystem,10 -Wl,--strip-all -Wl,-Map=$(@:.efi=.map) \
	  -Wl,--wrap=memcpy -Wl,--wrap=memset
LIBEFI = gnu-efi/x86_64/lib/libefi.a
LIBACPICA = libacpica.a
BDF2CFLAGS = PUA=0 SP=0 BRAILLE=0

ifneq "" "$(SBSIGN_MOK)"
default: truckload.signed.efi truckload.efi
else
default: truckload.efi
endif
.PHONY: default

ifneq "" "$(SBSIGN_MOK)"
truckload.signed.efi: truckload.efi
	sbsign --key $(SBSIGN_MOK:=.key) --cert $(SBSIGN_MOK:=.crt) \
	       --output $@ $<
endif

truckload.efi: start.o efi-main.o acpi.o acpica-osl.o apic.o fb-con.o \
    fb-con-cprintf.o font-default.o lm86-rm86.o mem-heap.o mem-map.o \
    panic.o stage1.o stage2.o stage3.o $(LIBEFI) $(LIBACPICA) \
    crt/ctype.o crt/mbtowc.o crt/memcmp.o crt/memmove.o crt/memset.o \
    crt/strcpy.o crt/strlen.o crt/strncat.o crt/strncmp.o crt/strncpy.o \
    truckload.ld
	$(CC) $(LDFLAGS) -o $@ $(^:%.ld=-T %.ld) $(LDLIBS)

%.o: %.c $(LIBEFI) font-default.h
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

# For debugging.
%.s: %.c $(LIBEFI) font-default.h
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $(CPPFLAGS) -S -o $@ $<

font-default.c: $(UNIVGASRCDIR)/u_vga16.bdf $(BDF2CSRCDIR)/bdf2c.awk
	$(BDF2CSRCDIR)/bdf2c.awk $(BDF2CFLAGS) $< >$@.tmp
	mv $@.tmp $@

font-default.h: $(UNIVGASRCDIR)/u_vga16.bdf $(BDF2CSRCDIR)/bdf2c.awk
	$(BDF2CSRCDIR)/bdf2c.awk H=1 $(BDF2CFLAGS) $< >$@.tmp
	mv $@.tmp $@

efi-main.o fb-con.o : CPPFLAGS += -DPACKAGE_NAME='"$(conf_Pkg_name)"' \
				  -DPACKAGE_VERSION='"$(conf_Pkg_ver)"'

$(LIBEFI):
	mkdir -p gnu-efi
	$(MAKE) CROSS_COMPILE=x86_64-w64-mingw32- CFLAGS='$(CFLAGS)' \
	    -C gnu-efi \
	    -f '$(abspath $(conf_Srcdir))'/gnu-efi/Makefile \
	    lib inc

ACPICAOBJS := $(patsubst $(ACPICASRCDIR)/%.c,acpica/%.o, \
		  $(wildcard $(ACPICASRCDIR)/source/components/dispatcher/*.c \
			     $(ACPICASRCDIR)/source/components/events/*.c \
			     $(ACPICASRCDIR)/source/components/executer/*.c \
			     $(ACPICASRCDIR)/source/components/hardware/*.c \
			     $(ACPICASRCDIR)/source/components/namespace/*.c \
			     $(ACPICASRCDIR)/source/components/parser/*.c \
			     $(ACPICASRCDIR)/source/components/tables/*.c \
			     $(ACPICASRCDIR)/source/components/utilities/*.c))

$(LIBACPICA): $(ACPICAOBJS)
	$(RM) $@
	$(AR) cqs $@.tmp $^
	mv $@.tmp $@

$(ACPICAOBJS) : %.o: %.c acpica-osl-inl.h
$(ACPICAOBJS) : CPPFLAGS += -DACPI_USE_LOCAL_CACHE -DACPI_DEBUG_OUTPUT \
			    -include $(conf_Srcdir)/acpica-osl-inl.h

distclean: clean
	$(RM) config.cache
ifeq "$(conf_Separate_build_dir)" "yes"
	-$(RM) GNUmakefile
endif
.PHONY: distclean

clean:
	$(RM) *.[soda] crt/*.[soda] *.so *.efi *.map \
	    font-default.c font-default.h *~ crt/*~
ifeq "$(conf_Separate_build_dir)" "yes"
	$(RM) -r gnu-efi acpica
else
	$(MAKE) -C gnu-efi clean
	$(RM) $(ACPICAOBJS)
endif
.PHONY: clean

-include *.d
