-include config.cache
conf_Lolwutconf_dir ?= lolwutconf
-include $(conf_Lolwutconf_dir)/lolwutconf.mk

CFLAGS = -Os -Wall -mno-red-zone -fno-stack-protector -fshort-wchar
LDFLAGS := -L $(conf_Gnuefi_dir)/lib -T elf_x86_64_efi.lds -shared -Bsymbolic

ifneq "" "$(SBSIGN_MOK)"
default: rebrief.signed.efi rebrief.efi
else
default: rebrief.efi
endif
.PHONY: default

ifneq "" "$(SBSIGN_MOK)"
rebrief.signed.efi: rebrief.efi
	sbsign --key $(SBSIGN_MOK:=.key) --cert $(SBSIGN_MOK:=.crt) \
	       --output $@ $<
endif

rebrief.efi: rebrief.so
	$(OBJCOPY) -j .text -j .sdata -j .data -j .dynamic -j .dynsym \
	    -j '.rel*' --target=efi-app-x86_64 $< $@

rebrief.so: rebrief.o
	$(LD) $(LDFLAGS) -o $@ -l:crt0-efi-x86_64.o $^ $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

distclean: clean
	$(RM) config.cache
ifeq "$(conf_Separate_build_dir)" "yes"
	-$(RM) GNUmakefile
endif
.PHONY: distclean

clean:
	$(RM) *.o *.so *.efi *~
.PHONY: clean
