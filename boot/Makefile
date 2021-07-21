# $@ = target file
# $< = first dependency
# $^ = all dependencies

DISK_IMAGE = device/testOS.img
MOUNT_POINT = /dev/nbd0
DEVICE = $(MOUNT_POINT)p1
ARGS = "file=$(DISK_IMAGE),format=raw,index=0,media=disk"
# Floppy: ARGS = "file=$(BIN),format=raw,index=0,if=floppy"

LINUX_LIB = ../linuxLib
LIB_DIR = ../lib
LIBS = -lc
INC = ../include
INC_SYS = $(INC)/sys
INCLUDE = -I$(INC) -I$(INC_SYS)
C_FLAGS = -c -m32 -D_M16 -ffreestanding -Wall -Werror
LD_FLAGS = -Ttext 0x0 -m elf_i386 -nostdLIB -nmagic $(LIBS) -L$(LIB_DIR)
OC_FLAGS = -O binary -j .text -j .rodata -j .data

C_SOURCE = $(wildcard *.c)
HEADERS = $(wildcard *.h, $(INC)/*.h, $(INC_SYS)/*.h)
OBJ = ${C_SOURCE : .c = .o}



all: install run

run: 
	qemu-system-i386 -drive $(ARGS)

%.o: %.c $(HEADERS)
	gcc $(C_FLAGS) $(INCLUDE) $< -o $@

%.o: %.s
	as --32 $< -o $@

%.bin: %.asm
	nasm -f bin $< -o $@

boot.elf: \
		boothead.o \
		util.o \
		kprintf.o \
		memory.o \
		memUtil.o \
		boot.o 
	ld $^ $(LD_FLAGS) -o $@ 

boot.bin: boot.elf
	objcopy $(OC_FLAGS) $< $@ 

installboot: installboot.c 
	gcc -o $@ $< -I$(LINUX_LIB) $(LINUX_LIB)/*.o

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
