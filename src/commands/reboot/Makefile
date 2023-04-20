MY_HOME = ../..
include $(MY_HOME)/common.mk
CFLAGS += -D_MINIX -D_POSIX_SOURCE

all: shutdown.bin \
	 halt.bin \
	 tinyhalt.bin

shutdown.bin: $(CRT_ENTRY) shutdown.o sh_wall.o log.o
	$(call mkbin,$^,$@,4kw,debug_$@)

halt.bin: $(CRT_ENTRY) halt.o log.o
	$(call mkbin,$^,$@,4kw,debug_$@)

tinyhalt.bin: $(CRT_ENTRY) tinyhalt.o
	$(call mkbin,$^,$@,4kw,debug_$@)

clean: 
	$(call cleanCommon)

disasm: 
	$(MK) NAME=$(NAME) disasm

