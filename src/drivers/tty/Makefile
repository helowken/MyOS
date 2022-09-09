MY_HOME = ../..
include $(MY_HOME)/common.mk
LIBS = -L$(MY_HOME)/lib \
	   -lmysys -lmysysutil -lmytimers -lmyc 

OBJS = $(ENTRY) tty.o console.o vidcopy.o keyboard.o rs232.o pty.o
DRIVER = tty.bin
STACK_SIZE = 1024

all: $(DRIVER)

$(DRIVER): $(OBJS) 
	$(call link,$^,$(LIBS),$@) 
	$(call createDebug,$@,$(DEBUG))
	$(call setStack,$(STACK_SIZE),$(DRIVER))

clean:
	$(call cleanCommon)

disasm: $(DEBUG)
	$(call disasmCode,$<,32)

