# biᴇꜰɪrcate

_very experimental_

 1. &nbsp;`sudo apt-get install gcc-mingw-w64-x86-64 dosfstools mtools`
 2. &nbsp;`sudo apt-get install qemu-system-x86 qemu-utils ovmf`
 3. &nbsp;`./configure`
 4. &nbsp;`make -j4`
 5. &nbsp;`make run-qemu-xv6`

This aims to run x86-16 or x86-32 code from an x86-64 UEFI environment.

Currently the bootloader can run an unmodified kernel from MIT's [Xv6](https://github.com/mit-pdos/xv6-public) teaching operating system &mdash; bypassing Xv6's own legacy BIOS bootloader &mdash; on a QEMU virtual machine with serial console.
