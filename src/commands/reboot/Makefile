MY_HOME = ../..
include $(MY_HOME)/common.mk
CFLAGS += -D_MINIX -D_POSIX_SOURCE

all: shutdown.bin

shutdown.bin: $(CRT_ENTRY) shutdown.o sh_wall.o log.o
	$(call mkbin,$^,$@,4kw,debug_shutdown.bin)

clean: 
	$(call cleanCommon)

disasm: 
	$(MK) NAME=$(NAME) disasm

