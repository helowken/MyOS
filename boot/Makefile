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

testBoot.bin: testBoot.asm
	nasm -f bin $< -o $@

installboot: installboot.c 
	gcc -o $@ $< -I$(lib) $(lib)/*.o

install: install_masterboot install_bootable install_testBoot

install_masterboot: installboot masterboot.bin
	./installboot -m $(diskImg) masterboot.bin

install_bootable: installboot bootblock.bin
	sudo ./installboot -b $(device) bootblock.bin

install_testBoot: installboot testBoot.bin
	sudo ./installboot -t $(device) testBoot.bin

run: masterboot.bin
	qemu-system-i386 -drive $(ARGS)

mount:
	sudo modprobe nbd
	sudo qemu-nbd --format=raw --connect=$(diskMount) $(diskImg)

clean:
	rm -f *.o *.bin instasllboot
