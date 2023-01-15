MY_HOME = ../..
include $(MY_HOME)/common.mk

DRIVER_DIR = ../libdriver

OBJS = $(ENTRY) log.o diag.o kputc.o
LIB_DRIVER = $(DRIVER_DIR)/driver.o 
DRIVER = log.bin
STACK_SIZE = 4kb

all: $(DRIVER)

$(DRIVER): $(OBJS) $(LIB_DRIVER)
	$(call link,$^,$(LIBS),$@) 
	$(call createDebug,$@,$(DEBUG))
	$(call setStack,$(STACK_SIZE),$@)

$(LIB_DRIVER):
	cd $(DRIVER_DIR) && $(MAKE)

clean:
	$(call cleanCommon)

disasm: $(DEBUG)
	$(call disasmCode,$<,32)

