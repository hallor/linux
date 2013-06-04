#!/bin/sh
echo "Copying kernel sources"
cp vmlinux vmlinux.strip
echo "Stripping sources"
mips-linux-gnu-strip --strip-debug vmlinux.strip
echo "Building uboot image"
mkimage -A mips -T kernel -C none -a 0x80000000 -e `mips-linux-gnu-objdump -f vmlinux.strip | grep start | cut -d " " -f 3` -n "Linux" -d vmlinux.strip uImage
