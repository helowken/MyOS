# $@ = target file
# $< = first dependency
# $^ = all dependencies

disk=device/testOS.img
device=/dev/nbd0p1
lib=../lib
all: clean install run

#for floppy:  ARGS="file=$(BIN),format=raw,index=0,if=floppy"
ARGS="file=$(disk),format=raw,index=0,media=disk"

masterboot.bin: masterboot.asm
	nasm -f bin $< -o $@

bootblock.bin: bootblock.asm
	nasm -f bin $< -o $@

installboot: installboot.c 
	gcc -o $@ $< -I$(lib) $(lib)/*.o

install: install_masterboot install_bootable

install_masterboot: installboot masterboot.bin
	./installboot -m $(disk) masterboot.bin

install_bootable: installboot bootblock.bin
	sudo ./installboot -b $(device) bootblock.bin

run: masterboot.bin
	qemu-system-i386 -drive $(ARGS)

clean:
	rm -f *.o *.bin instasllboot
