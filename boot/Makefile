MY_HOME = ..
MODE = M16
include $(MY_HOME)/common.mk
LIB_DIR = $(MY_HOME)/lib/m16
LIBS = -lmyc -lmysysutil
DISK_IMAGE = device/testOS.img
# Floppy: ARGS = "file=$(BIN),format=raw,index=0,if=floppy"
LINUX_LIB = $(MY_HOME)/linuxLib
INSTALL_BOOT = installboot.bin


all: install run

run: 
	qemu-system-i386 -drive "file=$(DISK_IMAGE),format=raw,index=0,media=disk"

%.bin: %.asm
	nasm -f bin $< -o $@

boot.elf: \
		boothead.o \
		util.o \
		boot.o \
		bootimage.o
	# -nmagic: merge text and data
	$(LD) $^ -Ttext 0x0 -m elf_i386 -nostdlib -L$(LIB_DIR) $(LIBS) -o $@ 

boot.bin: boot.elf
	objcopy -O binary -j .text -j .rodata -j .data $< $@ 

installboot: installboot.c 
	$(CC) -o $(INSTALL_BOOT) $< -I$(LINUX_LIB) $(LINUX_LIB)/*.o

install: \
	install_masterboot \
	install_device \
	install_bootable \
	install_image

install_masterboot: masterboot.bin installboot
	./$(INSTALL_BOOT) -m $(DISK_IMAGE) $<

install_device: bootblock.bin boot.bin installboot
	./$(INSTALL_BOOT) -d $(DISK_IMAGE) $< boot.bin 

install_bootable: boot.bin installboot
	./$(INSTALL_BOOT) -b $(DISK_IMAGE) $<

install_image: installboot
	./$(INSTALL_BOOT) -i $(DISK_IMAGE) ../kernel/kernel

reset:
	cd device && ./run && cd ..

clean:
	rm -f *.o *.bin *.elf 
