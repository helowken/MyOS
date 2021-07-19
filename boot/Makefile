# $@ = target file
# $< = first dependency
# $^ = all dependencies

DISK_IMAGE = device/testOS.img
MOUNT_POINT = /dev/nbd0
DEVICE = $(MOUNT_POINT)p1
LIB = ../lib
INCLUDE = include
ARGS = "file=$(DISK_IMAGE),format=raw,index=0,media=disk"
# Floppy: ARGS = "file=$(BIN),format=raw,index=0,if=floppy"

C_SOURCE = $(wildcard *.c)
HEADERS = $(wildcard $(INCLUDE)/*.h)
OBJ = ${C_SOURCE : .c = .o}


all: install run

run: all
	qemu-system-i386 -drive $(ARGS)

%.o: %.c $(HEADERS)
	gcc -c -m32 -ffreestanding -Wall -Werror -I$(INCLUDE) $< -o $@

%.o: %.s
	as --32 $< -o $@

%.bin: %.asm
	nasm -f bin $< -o $@

boot.elf: boothead.o print/print_string.o util.o boot.o 
	ld -Ttext 0x0 -m elf_i386 -nostdLIB -nmagic $^ -o $@ 

boot.bin: boot.elf
	objcopy -O binary -j .text -j .rodata $< $@ 

installboot: installboot.c 
	gcc -o $@ $< -I$(LIB) $(LIB)/*.o

install: \
	install_masterboot \
	install_device \
	install_boot

install_masterboot: masterboot.bin installboot 
	./installboot -m $(DISK_IMAGE) $<

install_device: bootblock.bin boot.bin installboot 
	sudo ./installboot -d $(DEVICE) $< boot.bin

install_boot: boot.bin installboot
	sudo ./installboot -b $(DEVICE) $<

mount:
	sudo modprobe nbd
	sudo qemu-nbd --format=raw --connect=$(MOUNT_POINT) $(DISK_IMAGE)

clean:
	rm -f *.o *.bin *.elf installboot
	rm -f print/*.o
