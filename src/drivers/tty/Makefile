MY_HOME = ../..
include $(MY_HOME)/common.mk

OBJS = $(ENTRY) tty.o console.o vidcopy.o keyboard.o rs232.o pty.o
DRIVER = tty.bin
STACK_SIZE = 256w
DIR_KEYMAPS = keymaps

all: install_keymaps $(DRIVER)

$(DRIVER): $(OBJS) 
	$(call mkSysBin,$^,$@,$(STACK_SIZE))

install_keymaps:
	cd $(DIR_KEYMAPS) && $(MAKE)

clean:
	cd $(DIR_KEYMAPS) && $(MAKE) $@
	$(call cleanCommon)

disasm: $(DEBUG)
	$(call disasmCode,$<,32)

