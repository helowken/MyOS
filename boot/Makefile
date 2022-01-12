MY_HOME = ..
MODE = M16
include $(MY_HOME)/common.mk

DISK_IMAGE = device/testOS.img
# Floppy: ARGS = "file=$(BIN),format=raw,index=0,if=floppy"
LIBS = -L$(MY_HOME)/lib/m16 \
	   -lmyc -lmysysutil
INSTALL = $(MY_HOME)/tools/installboot.bin

PROGRAMS = ../kernel/kernel.bin \
		   ../servers/pm/pm.bin

all: install run

run: 
	#qemu-system-i386 -drive "file=$(DISK_IMAGE),format=raw,index=0,media=disk"
	bochs

gdb:
	/usr/local/bin/bochs -q -f .bochsrc_gdb

%.bin: %.asm
	nasm -f bin $< -o $@

boot.elf: \
		boothead.o \
		util.o \
		boot.o \
		bootimage.o
	$(call link,$^,$(LIBS),$@) 
	$(call createDebug,$@,$(DEBUG))

boot.bin: boot.elf
	objcopy -O binary -j .text -j .rodata -j .data $< $@ 

install: \
	install_masterboot \
	install_device \
	install_bootable \
	install_image

install_masterboot: masterboot.bin 
	$(INSTALL) -m $(DISK_IMAGE) $<

install_device: bootblock.bin boot.bin 
	$(INSTALL) -d $(DISK_IMAGE) $< boot.bin 

install_bootable: boot.bin 
	$(INSTALL) -b $(DISK_IMAGE) $<

install_image: 
	$(INSTALL) -i $(DISK_IMAGE) $(PROGRAMS)

reset:
	cd device && ./run && cd ..

disasm: $(DEBUG)
	$(call disasmCode,$<,16)

clean:
	$(call cleanCommon)


