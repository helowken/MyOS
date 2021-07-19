# $@ = target file
# $< = first dependency
# $^ = all dependencies

DISK_IMAGE = device/testOS.img
MOUNT_POINT = /dev/nbd0
DEVICE = $(MOUNT_POINT)p1
LIB = ../lib
ARGS = "file=$(DISK_IMAGE),format=raw,index=0,media=disk"

C_SOURCE = $(wildcard *.c)
HEADERS = $(wildcard *.h)
OBJ = ${C_SOURCE : .c = .o}


all: install

run: all
	qemu-system-i386 -drive $(ARGS)

# for floppy:  ARGS="file=$(BIN),format=raw,index=0,if=floppy"

%.o: %.c $(HEADERS)
	gcc -c -m32 -ffreestanding -Wall -Werror $< -o $@

%.o: %.s
	as --32 $< -o $@

%.o: %.asm
	nasm -f elf $< -o $@

%.bin: %.asm
	nasm -f bin $< -o $@

testboot.elf: boothead.o print/print_string.o testboot.o 
	ld -Ttext 0x0 -m elf_i386 -nostdLIB -nmagic $^ -o $@ 

testboot.bin: testboot.elf
	objcopy -O binary -j .text -j .rodata $< $@ 

installboot: installboot.c 
	gcc -o $@ $< -I$(LIB) $(LIB)/*.o

install: \
	install_masterboot \
	install_bootable \
	install_testboot

install_masterboot: masterboot.bin installboot 
	./installboot -m $(DISK_IMAGE) $<

install_bootable: bootblock.bin installboot 
	sudo ./installboot -b $(DEVICE) $<

install_testboot: testboot.bin installboot
	sudo ./installboot -t $(DEVICE) $<

install_testboot2:
	gcc -c -m32 test_code16.c 
	as --32 ttt.s -o ttt.o
	ld -Ttext 0x0 -m elf_i386 ttt.o test_code16.o -o test.elf
	objcopy -j .text -j .rodata -O binary test.elf test.bin
	sudo ./installboot -t $(DEVICE) test.bin

mount:
	sudo modprobe nbd
	sudo qemu-nbd --format=raw --connect=$(MOUNT_POINT) $(DISK_IMAGE)

clean:
	rm -f *.o *.bin *.elf installboot
	rm -f print/*.o
