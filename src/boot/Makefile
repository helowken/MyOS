MY_HOME = ..
MODE = M16
include $(MY_HOME)/common.mk

DIR_DEVICE = device
DIR_TOOLS = $(MY_HOME)/tools
DISK_IMAGE = $(DIR_DEVICE)/testOS.img
LIBS = -L$(MY_HOME)/lib/m16 \
	   -lmyc -lmysysutil
INSTALL = $(DIR_TOOLS)/installboot.bin
MKFS = $(DIR_TOOLS)/mkfs.bin
BOOT = boot.bin
ENV_MODULES_FILE=$(MY_HOME)/env_modules

PROGRAMS = ../kernel/kernel.bin \
		   ../drivers/tty/tty.bin 
		   #../drivers/memory/memory.bin 
		   #../servers/pm/pm.bin 
		   #AT:../drivers/at_wini/at_wini.bin

all: compile

# Please see README for how to install Bochs.
# Need to use a Bochs without gdb support.
run: 
	$(BOCHS_NORMAL)/bin/bochs -q -f .bochsrc

# Need to use a Bochs with gdb support.
gdb:
	$(BOCHS_GDB)/bin/bochs -q -f .bochsrc_gdb

compile: masterboot.bin \
		 bootblock.bin \
		 $(BOOT)

%.bin: %.asm
	nasm -f bin $< -o $@

boot.elf: \
		boothead.o \
		util.o \
		boot.o \
		rawfs.o \
		bootimage.o
	$(call link,$^,$(LIBS),$@) 
	$(call createDebug,$@,$(DEBUG))

$(BOOT): boot.elf
	objcopy -O binary -j .text -j .rodata -j .data $< $@ 

install: reset \
	install_masterboot \
	install_device 

install_masterboot: masterboot.bin 
	$(INSTALL) -m $(DISK_IMAGE) $<

install_device: bootblock.bin $(BOOT)
	$(INSTALL) -d $(DISK_IMAGE) $< $(BOOT) $(ENV_MODULES_FILE) $(PROGRAMS)

reset:
	cd $(DIR_DEVICE) && ./run && cd ..
	$(MKFS) $(DISK_IMAGE)

disasm: $(DEBUG)
	$(call disasmCode,$<,16)

clean:
	$(call cleanCommon)


