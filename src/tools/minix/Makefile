MY_HOME = ../..
include $(MY_HOME)/common.mk
CFLAGS += -D_MINIX -D_POSIX_SOURCE 
LIBS = -L$(MY_HOME)/lib -lmyc -le
NAME = mkfs
BIN = $(NAME).bin
STACK_SIZE = 20kw

all: $(BIN)

$(BIN): $(CRT_ENTRY) ../minix_mkfs.o ../minix_util.o ../minix_error_functions.o
	$(call mkbin,$^,$@,$(STACK_SIZE),$(DEBUG))

clean:
	rm -f *.bin ../minix_*.o

disasm: $(DEBUG)
	$(call disasmCode,$<,32)
