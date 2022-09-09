MY_HOME = ../..
include $(MY_HOME)/common.mk
LIBS = -L$(MY_HOME)/lib \
	   -lmysys -lmysysutil -lmytimers -lmyc 

DRIVER_DIR = ../libdriver

OBJS = $(ENTRY) log.o diag.o kputc.o
LIB_DRIVER = $(DRIVER_DIR)/driver.o 
DRIVER = log.bin
STACK_SIZE = 4096

all: $(DRIVER)

$(DRIVER): $(OBJS) $(LIB_DRIVER)
	$(call link,$^,$(LIBS),$@) 
	$(call createDebug,$@,$(DEBUG))
	$(call setStack,$(STACK_SIZE),$(DRIVER))

$(LIB_DRIVER):
	cd $(DRIVER_DIR) && $(MAKE)

clean:
	$(call cleanCommon)

disasm: $(DEBUG)
	$(call disasmCode,$<,32)

