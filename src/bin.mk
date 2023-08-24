include $(MY_HOME)/common.mk
CFLAGS += -D_MINIX -D_POSIX_SOURCE 
LIBS = -L$(MY_HOME)/lib -lmyc -le
BIN = $(NAME).bin
DEBUG = debug_$(NAME).bin

all: $(BIN)

$(BIN): $(CRT_ENTRY) $(NAME).o
	$(call mkbin,$^,$@,$(STACK_SIZE),$(DEBUG))

clean:
	rm -f $(NAME).o $(BIN) $(DEBUG) 

disasm: $(DEBUG)
	$(call disasmCode,$<,32)
