# $@ = target file
# $< = first dependency
# $^ = all dependencies

diskImg=device/testOS.img
diskMount=/dev/nbd0
device=$(diskMount)p1
lib=../lib


all: clean install run

# for floppy:  ARGS="file=$(BIN),format=raw,index=0,if=floppy"

ARGS="file=$(diskImg),format=raw,index=0,media=disk"

masterboot.bin: masterboot.asm
	nasm -f bin $< -o $@

bootblock.bin: bootblock.asm
	nasm -f bin $< -o $@

testboot.o : testboot.c code16gcc.h
	gcc -c -g -Os -m16 -march=i686 -ffreestanding -Wall -Werror -I. -o $@ $<

testboot.elf: testboot.o
	ld -static -m elf_i386 -Tlinker.ld -nostdlib --nmagic -o $@ $<

testboot.bin: testboot.elf
	objcopy -O binary $< $@ 

boothead.bin: boothead.asm
	nasm -f bin $< -o $@

installboot: installboot.c 
	gcc -o $@ $< -I$(lib) $(lib)/*.o

install: install_masterboot install_bootable install_testboot

install_masterboot: installboot masterboot.bin
	./installboot -m $(diskImg) masterboot.bin

install_bootable: installboot bootblock.bin
	sudo ./installboot -b $(device) bootblock.bin

install_testboot: installboot testboot.bin
	sudo ./installboot -t $(device) testboot.bin

run: masterboot.bin
	qemu-system-i386 -drive $(ARGS)

mount:
	sudo modprobe nbd
	sudo qemu-nbd --format=raw --connect=$(diskMount) $(diskImg)

clean:
	rm -f *.s *.o *.bin instasllboot
