MY_HOME = ..
MODE = M16
include $(MY_HOME)/common.mk

DIR_DEVICE = device
DISK_IMAGE = $(DIR_DEVICE)/testOS.img
LIBS = -L$(MY_HOME)/lib/m16 \
	   -lmyc -lmysysutil
INSTALL = $(MY_HOME)/tools/installboot.bin
BOOT = boot.bin

PROGRAMS = ../kernel/kernel.bin \
		   ../servers/pm/pm.bin \
		   AT:../drivers/at_wini/at_wini.bin

all: install run

# Please see README for how to install Bochs.
# Need to use a Bochs without gdb support.
run: 
	bochs

# Need to use a Bochs with gdb support.
gdb:
	/home/helowken/bochs/build/bin/bochs -q -f .bochsrc_gdb

%.bin: %.asm
	nasm -f bin $< -o $@

boot.elf: \
		boothead.o \
		util.o \
		boot.o \
		bootimage.o
	$(call link,$^,$(LIBS),$@) 
	$(call createDebug,$@,$(DEBUG))

$(BOOT): boot.elf
	objcopy -O binary -j .text -j .rodata -j .data $< $@ 

install: \
	install_masterboot \
	install_device \
	install_bootable \
	install_image

install_masterboot: masterboot.bin 
	$(INSTALL) -m $(DISK_IMAGE) $<

install_device: bootblock.bin $(BOOT)
	$(INSTALL) -d $(DISK_IMAGE) $< $(BOOT)

install_bootable: $(BOOT)
	$(INSTALL) -b $(DISK_IMAGE) $<

install_image: 
	$(INSTALL) -i $(DISK_IMAGE) $(PROGRAMS)

reset:
	cd $(DIR_DEVICE) && ./run && cd ..

disasm: $(DEBUG)
	$(call disasmCode,$<,16)

clean:
	$(call cleanCommon)


