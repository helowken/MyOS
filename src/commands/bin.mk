MY_HOME = ../..
include $(MY_HOME)/common.mk
CFLAGS += -D_MINIX -D_POSIX_SOURCE 
LIBS = -L$(MY_HOME)/lib -lmyc
DST_DIR = $(RES_DIR)/bin
BIN = $(NAME).bin
DEBUG = debug_$(NAME).bin

all: $(BIN)

$(BIN): $(CRT_ENTRY) $(NAME).o
	$(call mkbin,$^,$@,$(STACK_SIZE),$(DEBUG),$(DST_DIR))

clean:
	rm -f $(NAME).o $(BIN) $(DEBUG) $(DST_DIR)/$(BIN)

disasm: $(DEBUG)
	$(call disasmCode,$<,32)
