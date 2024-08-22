#!/bin/sh
gcc -o "$(basename "$1" .c)" -static "$1"
mv "$(basename "$1" .c)" ./initramfs
cd initramfs
find . -print0 \
| cpio --null -ov --format=newc \
| gzip -9 > initramfs.cpio.gz
mv ./initramfs.cpio.gz ../
cd ..

qemu-system-x86_64 \
    -m 256M \
    -cpu kvm64,+smep\
    -kernel vmlinuz \
    -initrd initramfs.cpio.gz \
    -hdb flag.txt \
    -snapshot \
    -nographic \
    -monitor /dev/null \
    -no-reboot \
    -s \
    -append "console=ttyS0 kaslr kpti=1 quiet panic=1 nosmap"
