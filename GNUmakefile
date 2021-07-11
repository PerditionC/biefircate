ifeq "" "$(wildcard config.cache)"
$(error you must configure this project first!)
endif

-include config.cache
-include $(conf_Lolwutconf_dir)/lolwutconf.mk

GNUEFISRCDIR := '$(abspath $(conf_Srcdir))'/gnu-efi
CFLAGS = -pie -fPIC -ffreestanding -Os -Wall -mno-red-zone -fno-stack-protector -MMD
CPPFLAGS = -I $(GNUEFISRCDIR)/inc -I $(GNUEFISRCDIR)/protocol \
	   -I $(GNUEFISRCDIR)/inc/x86_64
LDFLAGS = $(CFLAGS) -nostdlib -ffreestanding -Wl,--entry,efi_main \
	  -Wl,--subsystem,10 -Wl,--strip-all -Wl,-Map=$(@:.efi=.map)
LIBEFI = gnu-efi/x86_64/lib/libefi.a
LDLIBS := $(LIBEFI) $(LDLIBS)

ifneq "" "$(SBSIGN_MOK)"
default: loader.signed.efi loader.efi
else
default: loader.efi
endif
.PHONY: default

ifneq "" "$(SBSIGN_MOK)"
loader.signed.efi: loader.efi
	sbsign --key $(SBSIGN_MOK:=.key) --cert $(SBSIGN_MOK:=.crt) \
	       --output $@ $<
endif

loader.efi: efi-main.o rm86.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.c $(LIBEFI)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

efi-main.o : CPPFLAGS += -DVERSION='"$(conf_Pkg_ver)"'

# gnu-efi's Make.defaults has a bit of a bug in its setting of $(GCCVERSION)
# & $(GCCMINOR): if $(CC) -dumpversion says something like `10-win32' it
# fails to clip off the `-win32' part.  This later leads to incorrect output
# code.  Work around this here.
$(LIBEFI):
	mkdir -p gnu-efi
	$(MAKE) CROSS_COMPILE=x86_64-w64-mingw32- CFLAGS='$(CFLAGS)' \
	    GCCVERSION=$(shell $(CC) -dumpversion | cut -f1 -d. | cut -f1 -d-)\
	    GCCMINOR=$(shell $(CC) -dumpversion | cut -f2 -d. | cut -f1 -d-) \
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
	$(RM) *.[od] *.so *.efi *.map *~
ifeq "$(conf_Separate_build_dir)" "yes"
	$(RM) -r gnu-efi
else
	$(MAKE) -C gnu-efi clean
endif
.PHONY: clean

-include *.d
