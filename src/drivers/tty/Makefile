MY_HOME = ../..
include $(MY_HOME)/common.mk
LIBS += -lmytimers

OBJS = $(ENTRY) tty.o console.o vidcopy.o keyboard.o rs232.o pty.o
DRIVER = tty.bin
STACK_SIZE = 256w

all: install_keymaps $(DRIVER)

$(DRIVER): $(OBJS) 
	$(call link,$^,$(LIBS),$@) 
	$(call createDebug,$@,$(DEBUG))
	$(call setStack,$(STACK_SIZE),$@)

install_keymaps:
	cd keymaps && $(MAKE)

clean:
	cd keymaps && $(MAKE) $@
	$(call cleanCommon)

disasm: $(DEBUG)
	$(call disasmCode,$<,32)

