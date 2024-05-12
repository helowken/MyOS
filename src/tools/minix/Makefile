MY_HOME = ../..
include $(MY_HOME)/common.mk
CFLAGS += -D_MINIX -D_POSIX_SOURCE 
LIBS = -L$(MY_HOME)/lib -lmyc -le

all: mkfs.bin installboot.bin

mkfs.bin: $(CRT_ENTRY) ../minix_mkfs.o ../minix_util.o ../minix_error_functions.o
	$(call mkbin,$^,$@,20kw,$(DEBUG))

installboot.bin: $(CRT_ENTRY) ../minix_installboot.o ../minix_util.o ../minix_error_functions.o rawfs.o
	$(call mkbin,$^,$@,6kw,$(DEBUG))

clean:
	rm -f *.bin ../minix_*.o

disasm: $(DEBUG)
	$(call disasmCode,$<,32)
