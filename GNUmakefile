ifeq "" "$(wildcard config.cache)"
$(error you must configure this project first!)
endif

-include config.cache
-include $(conf_Lolwutconf_dir)/lolwutconf.mk

GNUEFISRCDIR := '$(abspath $(conf_Srcdir))'/gnu-efi
ACPICASRCDIR := '$(abspath $(conf_Srcdir))'/acpica
SPLEENSRCDIR := $(conf_Srcdir)/spleen
CFLAGS = -pie -fPIC -ffreestanding -Os -Wall -mno-red-zone -fno-stack-protector -MMD
CPPFLAGS = -I$(GNUEFISRCDIR)/inc \
	   -I$(GNUEFISRCDIR)/protocol \
	   -I$(GNUEFISRCDIR)/inc/x86_64 \
	   -iquote $(ACPICASRCDIR)/source/include \
	   -DGNU_EFI_USE_MS_ABI
LDFLAGS = $(CFLAGS) -nostdlib -ffreestanding -Wl,--entry,_start \
	  -Wl,--subsystem,10 -Wl,--strip-all -Wl,-Map=$(@:.efi=.map) \
	  -Wl,--wrap=memcpy -Wl,--wrap=memset
LIBEFI = gnu-efi/x86_64/lib/libefi.a
LDLIBS := $(LIBEFI) $(LDLIBS)
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

truckload.efi: start.o efi-main.o acpi.o exit.o fb-con.o font-default.o \
    lm86-rm86.o mem-map.o memcmp.o memmove.o memset.o stage1.o stage2.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.c $(LIBEFI) font-default.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

font-default.c: $(SPLEENSRCDIR)/spleen-8x16.bdf bdf2c.awk
	./bdf2c.awk $(BDF2CFLAGS) $< >$@.tmp
	mv $@.tmp $@

font-default.h: $(SPLEENSRCDIR)/spleen-8x16.bdf bdf2c.awk
	./bdf2c.awk H=1 $(BDF2CFLAGS) $< >$@.tmp
	mv $@.tmp $@

efi-main.o fb-con.o : CPPFLAGS += -DPACKAGE_NAME='"$(conf_Pkg_name)"' \
				  -DPACKAGE_VERSION='"$(conf_Pkg_ver)"'

$(LIBEFI):
	mkdir -p gnu-efi
	$(MAKE) CROSS_COMPILE=x86_64-w64-mingw32- CFLAGS='$(CFLAGS)' \
	    -C gnu-efi \
	    -f '$(abspath $(conf_Srcdir))'/gnu-efi/Makefile \
	    lib inc

distclean: clean
	$(RM) config.cache
ifeq "$(conf_Separate_build_dir)" "yes"
	-$(RM) GNUmakefile
endif
.PHONY: distclean

clean:
	$(RM) *.[od] *.so *.efi *.map font-default.c font-default.h *~
ifeq "$(conf_Separate_build_dir)" "yes"
	$(RM) -r gnu-efi
else
	$(MAKE) -C gnu-efi clean
endif
.PHONY: clean

-include *.d
