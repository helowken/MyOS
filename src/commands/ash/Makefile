MY_HOME = ../..
include $(MY_HOME)/common.mk
CFLAGS += -DSHELL -D_MINIX -D_POSIX_SOURCE
LIBS = -L$(MY_HOME)/lib \
	   -lmysysutil -lmysys -lmytimers -lmyc 
DST_DIR = $(MY_HOME)/tools/resources/bin

OBJS = main.o trap.o eval.o
STACK_SIZE = 102400
BIN = sh.bin

all: $(BIN)

$(BIN): $(ENTRY) $(OBJS)
	$(call link,$^,$(LIBS),$@) 
	$(call createDebug,$@,$(DEBUG))
	$(call setStack,$(STACK_SIZE),$(BIN))
	cp $@ $(DST_DIR)/$(NAME)

clean: 
	$(call cleanCommon)

disasm: $(DEBUG)
	$(call disasmCode,$<,32)
