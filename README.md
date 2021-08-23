# biᴇꜰɪrcate

_very experimental_ • _some [developer notes](NOTES.asciidoc) available_

 1. &nbsp;`sudo apt-get install gcc-mingw-w64-x86-64 gcc-multilib`
 2. &nbsp;`sudo apt-get install dosfstools mtools`
 3. &nbsp;`sudo apt-get install qemu-system-x86 qemu-utils ovmf`
 4. &nbsp;`./configure`
 5. &nbsp;`make -j4`
 6. &nbsp;`make run-qemu`

This aims to run x86-16 or x86-32 code from an x86-64 UEFI environment.

Currently the code tries to bring up any legacy option ROMs it can find, starting with the VGA option ROM.

The bootloader can now also run an unmodified kernel from MIT's [Xv6](https://github.com/mit-pdos/xv6-public) teaching operating system &mdash; bypassing Xv6's own legacy BIOS bootloader &mdash; on a QEMU virtual machine with serial console.  To build and run Xv6, also do these:

 7. &nbsp;`make -j4 hd-xv6.img`
 8. &nbsp;`make run-qemu-xv6`

Again, some [developer notes](NOTES.asciidoc) are available.
