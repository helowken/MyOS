MY_HOME = ..
include $(MY_HOME)/common.mk
CFLAGS += -D_M16

# Device
DIR_DEVICE = device
DIR_TOOLS = $(MY_HOME)/tools
DISK_IMAGE = $(DIR_DEVICE)/testOS.img
BOOT_IMG_SECTORS = 2048

# Partition
ROOT_MB = 16
PART_TYPE=81

# Make FS
PART_0 = p0
DEV_ROOT = $(PART_0)s0
DEV_HOME = $(PART_0)s1
DEV_USR = $(PART_0)s2
FS_ROOT = ROOT
FS_HOME = HOME
FS_USR = USR

# Program
LIBS = -L$(MY_HOME)/lib/m16 -lmyc -lmysysutil
INSTALL = $(DIR_TOOLS)/installboot.bin
MKFS = $(DIR_TOOLS)/mkfs.bin
PARTITION = $(DIR_TOOLS)/partition.bin
PARTSIZE = $(DIR_TOOLS)/partsize.bin
MASTER_BOOT = masterboot.bin
BOOT_BLOCK = bootblock.bin
BOOT = boot.bin
ENV_MODULES_FILE=$(MY_HOME)/env_modules

PROGRAMS = ../kernel/kernel.bin \
		   ../servers/pm/pm.bin \
		   ../servers/fs/fs.bin \
		   ../servers/rs/rs.bin \
		   ../drivers/tty/tty.bin \
		   ../drivers/memory/memory.bin \
		   ../drivers/log/log.bin \
		   AT:../drivers/at_wini/at_wini.bin \
		   ../servers/init/init.bin 

all: compile

$(DISK_IMAGE):
	cd $(DIR_DEVICE) && ./run && cd ..

# Please see README for how to install Bochs.
# Need to use a Bochs without gdb support.
run: rm_lock
	$(BOCHS_NORMAL)/bin/bochs -q -f .bochsrc

# Need to use a Bochs with gdb support.
gdb: rm_lock
	$(BOCHS_GDB)/bin/bochs -q -f .bochsrc_gdb

rm_lock:
	rm -f $(DISK_IMAGE).lock

%.bin: %.asm
	nasm -f bin $< -o $@

%.elf: %.asm
	nasm -f elf $< -o $@

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

compile: $(MASTER_BOOT) $(BOOT_BLOCK) $(BOOT)

install: install_masterboot install_device 

partition: $(DISK_IMAGE)
	$(eval PART_0_SIZE := $(shell $(PARTSIZE) $(DISK_IMAGE) 0))
	$(eval ROOT_SECTORS := $(shell expr $(ROOT_MB) '*' 1024 '*' 2))
	$(eval HOME_SECTORS := $(shell expr $(PART_0_SIZE) '/' 5 '/' 512))
	$(eval ROOT_PART := $(PART_TYPE):$(ROOT_SECTORS)*)
	$(eval HOME_PART := $(PART_TYPE):$(HOME_SECTORS))
	$(eval USR_PART := $(PART_TYPE):0+)
	$(PARTITION) -q $(DISK_IMAGE) 0 $(ROOT_PART) $(HOME_PART) $(USR_PART)

	$(eval PART_1_SIZE := $(shell $(PARTSIZE) $(DISK_IMAGE) 1))
	$(eval SUB_SECTORS := $(shell expr $(PART_1_SIZE) '/' 512 '/' 2))
	$(eval SUB_0_PART := $(PART_TYPE):$(SUB_SECTORS))
	$(eval SUB_1_PART := $(PART_TYPE):0+)
	$(PARTITION) -q $(DISK_IMAGE) 1 $(SUB_0_PART) $(SUB_1_PART)

	$(PARTITION) -ps $(DISK_IMAGE) 0
	$(PARTITION) -s $(DISK_IMAGE) 1

mkfs: partition
	@echo --------------------------------------------
	$(MKFS) -r$(BOOT_IMG_SECTORS) $(DEV_ROOT):$(DISK_IMAGE) $(FS_ROOT)
	$(MKFS) -p $(DEV_ROOT):$(DISK_IMAGE)
	@echo --------------------------------------------
	$(MKFS) $(DEV_HOME):$(DISK_IMAGE) $(FS_HOME)
	$(MKFS) -p $(DEV_HOME):$(DISK_IMAGE)
	@echo --------------------------------------------
	$(MKFS) $(DEV_USR):$(DISK_IMAGE) $(FS_USR)
	$(MKFS) -p $(DEV_USR):$(DISK_IMAGE)

install_masterboot: $(MASTER_BOOT) mkfs
	$(INSTALL) -m $(DISK_IMAGE) $<
	$(INSTALL) -m $(PART_0):$(DISK_IMAGE) $<

install_device: $(BOOT_BLOCK) $(BOOT) mkfs
	$(INSTALL) -d $(DEV_ROOT):$(DISK_IMAGE) $(BOOT_BLOCK) $(BOOT) \
		$(BOOT_IMG_SECTORS) $(ENV_MODULES_FILE) $(PROGRAMS)

disasm: $(DEBUG)
	$(call disasmCode,$<,16)

disasm_mbr: masterboot.elf
	$(call disasmCode,$<,16)

disasm_bb: bootblock.elf
	$(call disasmCode,$<,16)

clean:
	$(call cleanCommon)
	rm -f $(DISK_IMAGE)
	rm -f eth_null*



