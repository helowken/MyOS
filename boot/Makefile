MY_HOME = ..
MODE = M16
include $(MY_HOME)/common.mk
LIB_DIR = $(MY_HOME)/lib/m16
LIBS = -lmyc -lmysysutil
DISK_IMAGE = device/testOS.img
MOUNT_POINT = /dev/nbd0
DEVICE = $(MOUNT_POINT)p1
# Floppy: ARGS = "file=$(BIN),format=raw,index=0,if=floppy"
LINUX_LIB = $(MY_HOME)/linuxLib


all: install run

run: 
	qemu-system-i386 -drive "file=$(DISK_IMAGE),format=raw,index=0,media=disk"

%.bin: %.asm
	nasm -f bin $< -o $@

boot.elf: \
		boothead.o \
		util.o \
		boot.o 
	$(LD) $^ -Ttext 0x0 -m elf_i386 -nostdLIB -nmagic -L$(LIB_DIR) $(LIBS) -o $@ 

boot.bin: boot.elf
	objcopy -O binary -j .text -j .rodata -j .data $< $@ 

installboot: installboot.c 
	$(CC) -o $@ $< -I$(LINUX_LIB) $(LINUX_LIB)/*.o

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
