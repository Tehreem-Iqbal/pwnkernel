#!/bin/sh
gcc -o "$(basename "$1" .c)" -static "$1"
mv "$(basename "$1" .c)" ./initramfs 
cd initramfs
find . -print0 \
| cpio --null -ov --format=newc \
| gzip -9 > initramfs.cpio.gz
mv ./initramfs.cpio.gz ../
