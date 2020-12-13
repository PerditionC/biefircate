ifeq "" "$(wildcard config.cache)"
$(error you must configure this project first!)
endif

-include config.cache
-include $(conf_Lolwutconf_dir)/lolwutconf.mk

CFLAGS = -Os -Wall -mno-red-zone -fno-stack-protector -fshort-wchar
LDFLAGS := -L $(conf_Gnuefi_dir)/lib -T elf_x86_64_efi.lds -shared -Bsymbolic

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

loader.efi: loader.so
	$(OBJCOPY) -j .text -j .sdata -j .data -j .dynamic -j .dynsym \
	    -j '.rel*' --target=efi-app-x86_64 $< $@

loader.so: efi-main.o
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
