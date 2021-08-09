# Copyright (c) 2020--2021 TK Chia
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#   * Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#   * Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#   * Neither the name of the developer(s) nor the names of its
#     contributors may be used to endorse or promote products derived from
#     this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
# IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
# PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
# TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

ifeq "" "$(wildcard config.cache)"
$(error you must configure this project first!)
endif

-include config.cache
-include $(conf_Lolwutconf_dir)/lolwutconf.mk

GNUEFISRCDIR := '$(abspath $(conf_Srcdir))'/gnu-efi
CFLAGS = -pie -fPIC -ffreestanding -Os -Wall -mno-red-zone \
	 -fno-stack-protector -MMD
AS = nasm
ASFLAGS = -f win64 -MD $(@:.o=.d)
COMMON_CPPFLAGS = -DXV6_COMPAT
CPPFLAGS += -I $(GNUEFISRCDIR)/inc -I $(GNUEFISRCDIR)/protocol \
	    -I $(GNUEFISRCDIR)/inc/x86_64 -I $(conf_Srcdir) $(COMMON_CPPFLAGS)
LDFLAGS += $(CFLAGS) -nostdlib -ffreestanding -Wl,--entry,efi_main \
	  -Wl,--subsystem,10 -Wl,--strip-all -Wl,-Map=$(@:.efi=.map)
LIBEFI = gnu-efi/x86_64/lib/libefi.a
LDLIBS := $(LIBEFI) $(LDLIBS)

CFLAGS2 += -mregparm=3 -mrtd -ffreestanding -Os -Wall -fno-stack-protector -MMD
AS2 = nasm
ASFLAGS2 = -f elf32 -MD $(@:.o=.d)
CPPFLAGS2 += -I $(conf_Srcdir) $(COMMON_CPPFLAGS)
LDFLAGS2 += $(CFLAGS2) -static -nostdlib -ffreestanding \
    -Wl,--strip-all -Wl,-Map=$(@:.sys=.map) -Wl,--build-id=none

QEMUFLAGS = -m 224m -serial stdio
QEMUFLAGSXV6 = $(QEMUFLAGS) -hdb xv6/fs.img

ifneq "" "$(SBSIGN_MOK)"
STAGE1 = stage1.signed.efi
else
STAGE1 = stage1.efi
endif
STAGE2 = stage2.sys

default: $(STAGE1) $(STAGE2) hd.img romdumper.efi
.PHONY: default

ifneq "" "$(SBSIGN_MOK)"
stage1.signed.efi: stage1.efi
	sbsign --key $(SBSIGN_MOK:=.key) --cert $(SBSIGN_MOK:=.crt) \
	       --output $@ $<
endif

stage1.efi: stage1/main.o stage1/bmem.o stage1/bparm.o stage1/fv.o \
	    stage1/pci.o stage1/run-stage2.o stage1/util.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

stage1/%.o: stage1/%.c $(LIBEFI)
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

stage1/%.o: stage1/%.asm $(LIBEFI)
	mkdir -p $(@D)
	$(AS) $(ASFLAGS) $(CPPFLAGS) -o $@ $<

romdumper.efi: romdumper.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

romdumper.o: romdumper.c $(LIBEFI)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

stage1/main.o romdumper.o : CPPFLAGS += -DVERSION='"$(conf_Pkg_ver)"'

$(STAGE2): stage2/start.o stage2/mem.o stage2/stage2.ld
	$(CC2) $(LDFLAGS2) -o $@ $(^:%.ld=-T %.ld) $(LDLIBS2)

stage2/%.o: stage2/%.c
	mkdir -p $(@D)
	$(CC2) $(CFLAGS2) $(CPPFLAGS2) -c -o $@ $<

stage2/%.o: stage2/%.asm
	mkdir -p $(@D)
	$(AS2) $(ASFLAGS2) $(CPPFLAGS2) -o $@ $<

# gnu-efi's Make.defaults has a bit of a bug in its setting of $(GCCVERSION)
# & $(GCCMINOR): if $(CC) -dumpversion says something like `10-win32' it
# fails to clip off the `-win32' part.  This later leads to incorrect output
# code.  Work around this here.
$(LIBEFI):
	mkdir -p gnu-efi
	$(MAKE) CROSS_COMPILE=x86_64-w64-mingw32- CFLAGS='$(CFLAGS)' \
	    GCCVERSION=$(shell $(CC) -dumpversion | cut -f1 -d- | cut -f1 -d.)\
	    GCCMINOR=$(shell $(CC) -dumpversion | cut -f1 -d- | cut -f2 -d.) \
	    -C gnu-efi \
	    -f '$(abspath $(conf_Srcdir))'/gnu-efi/Makefile \
	    lib inc

xv6.stamp: $(conf_Srcdir)/xv6/Makefile
ifeq "$(conf_Separate_build_dir)" "yes"
	$(RM) -r xv6
	cp -a $(<D) xv6
endif
	$(MAKE) -C xv6 kernel fs.img
	>$@

hd.img: $(STAGE1) $(STAGE2)
	$(RM) $@.tmp
	dd if=/dev/zero of=$@.tmp bs=1048576 count=32
	echo start=32K type=0B bootable | sfdisk $@.tmp
	mkdosfs -v -F16 --offset 64 $@.tmp
	mmd -i $@.tmp@@32K ::/EFI ::/EFI/BOOT ::/EFI/biefirc
	mcopy -i $@.tmp@@32K $< ::/EFI/BOOT/bootx64.efi
	mcopy -i $@.tmp@@32K $(STAGE2) ::/EFI/biefirc/
	mv $@.tmp $@

hd.vdi: hd.img
	qemu-img convert $< -O vdi $@.tmp
	mv $@.tmp $@

hd-xv6.img: $(STAGE1) xv6.stamp
	$(RM) $@.tmp
	dd if=/dev/zero of=$@.tmp bs=1048576 count=32
	echo start=32K type=0B bootable | sfdisk $@.tmp
	mkdosfs -v -F16 --offset 64 $@.tmp
	mmd -i $@.tmp@@32K ::/EFI ::/EFI/BOOT
	mcopy -i $@.tmp@@32K $< ::/EFI/BOOT/bootx64.efi
	mcopy -i $@.tmp@@32K xv6/kernel ::/kernel.sys
	mv $@.tmp $@

distclean: clean
	$(RM) config.cache
ifeq "$(conf_Separate_build_dir)" "yes"
	-$(RM) GNUmakefile
endif
.PHONY: distclean

clean:
	set -e; \
	for d in . stage1 stage2; do \
		if test -d "$$d"; then \
			(cd "$$d" && \
			 $(RM) *.[od] *.so *.efi *.img *.vdi *.map *.stamp \
			       *.sys *~); \
		fi; \
	done
ifeq "$(conf_Separate_build_dir)" "yes"
	$(RM) -r stage1 stage2 gnu-efi xv6
else
	$(MAKE) -C gnu-efi clean
	$(MAKE) -C xv6 clean
endif
.PHONY: clean

run-qemu: hd.img
	qemu-system-x86_64 -bios /usr/share/ovmf/OVMF.fd -hda $< $(QEMUFLAGS)
.PHONY: run-qemu

run-qemu-xv6: hd-xv6.img xv6.stamp
	qemu-system-x86_64 -bios /usr/share/ovmf/OVMF.fd -hda $< \
	    $(QEMUFLAGSXV6)
.PHONY: run-qemu

-include *.d stage1/*.d stage2/*.d
