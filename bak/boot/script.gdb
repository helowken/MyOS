add-symbol-file testboot.elf 0x10000
target remote | qemu-system-i386 -S -gdb stdio -m 16 -drive "file=device/testOS.img,format=raw,index=0,media=disk"
