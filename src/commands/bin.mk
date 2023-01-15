MY_HOME = ../..
include $(MY_HOME)/common.mk
CFLAGS += -D_MINIX -D_POSIX_SOURCE 
LIBS += -lmytimers
DST_DIR = $(RES_DIR)/bin
BIN = $(NAME).bin

all: $(BIN)

$(BIN): $(CRT_ENTRY) $(NAME).o
	$(call link,$^,$(LIBS),$@) 
	$(call setStack,$(STACK_SIZE),$@)
	$(call copyTo,$@,$(DST_DIR))

clean:
	rm -f $(NAME).o $(BIN) $(DST_DIR)/$(BIN)

