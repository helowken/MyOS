MY_HOME = ..
MODE = M16
include $(MY_HOME)/common.mk

# Device
DIR_DEVICE = device
DIR_TOOLS = $(MY_HOME)/tools
DISK_IMAGE = $(DIR_DEVICE)/testOS.img
BOOT_IMG_SECTORS = 2048

# Partition
DEV_SIZE = $(shell stat -c%s $(DISK_IMAGE))
ROOT_MB = 16
ROOT_SECTORS = $(shell expr $(ROOT_MB) '*' 1024 '*' 2)
HOME_SECTORS = $(shell expr $(DEV_SIZE) '/' 5 '/' 512)
PART_TYPE = 81
ROOT_PART = $(PART_TYPE):$(ROOT_SECTORS)* 
HOME_PART = $(PART_TYPE):$(HOME_SECTORS)
USR_PART = $(PART_TYPE):0+

# Make FS
PRIMARY = p0
FS_ROOT = $(PRIMARY)s0
FS_HOME = $(PRIMARY)s1
FS_USR = $(PRIMARY)s2

# Program
LIBS = -L$(MY_HOME)/lib/m16 \
	   -lmyc -lmysysutil
INSTALL = $(DIR_TOOLS)/installboot.bin
MKFS = $(DIR_TOOLS)/mkfs.bin
PARTITION = $(DIR_TOOLS)/partition.bin
BOOT = boot.bin
ENV_MODULES_FILE=$(MY_HOME)/env_modules

PROGRAMS = ../kernel/kernel.bin \
		   ../servers/pm/pm.bin \
		   ../servers/fs/fs.bin \
		   ../drivers/tty/tty.bin \
		   ../drivers/memory/memory.bin \
		   ../drivers/log/log.bin \
		   AT:../drivers/at_wini/at_wini.bin

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

install: partition \
	mkfs \
	install_masterboot \
	install_device 

install_masterboot: masterboot.bin 
	$(INSTALL) -m $(DISK_IMAGE) $<
	$(INSTALL) -m $(PRIMARY):$(DISK_IMAGE) $<

install_device: bootblock.bin $(BOOT)
	$(INSTALL) -d $(FS_ROOT):$(DISK_IMAGE) $^ $(BOOT_IMG_SECTORS) \
		$(ENV_MODULES_FILE) $(PROGRAMS)

partition:
	cd $(DIR_DEVICE) && ./run && cd ..
	$(PARTITION) -q $(DISK_IMAGE) $(ROOT_PART) $(HOME_PART) $(USR_PART)
	$(PARTITION) -ps $(DISK_IMAGE)

mkfs:
	$(MKFS) -r$(BOOT_IMG_SECTORS) $(FS_ROOT):$(DISK_IMAGE)
	$(MKFS) $(FS_HOME):$(DISK_IMAGE)
	$(MKFS) $(FS_USR):$(DISK_IMAGE)

disasm: $(DEBUG)
	$(call disasmCode,$<,16)

clean:
	$(call cleanCommon)


