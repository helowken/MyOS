MY_HOME = ../..
include $(MY_HOME)/common.mk
LIBS = -L$(MY_HOME)/lib \
	   -lmysys -lmysysutil -lmyc 

DRIVER_DIR = ../libdriver

OBJS = $(ENTRY) memory.o 
LIB_DRIVER = $(DRIVER_DIR)/driver.o 
NAME = memory
DRIVER = $(NAME).bin
STACK_SIZE = 4096

all: $(DRIVER)

$(DRIVER): $(OBJS) $(LIB_DRIVER)
	$(call link,$^,$(LIBS),$@) 
	$(call createDebug,$@,$(DEBUG))
	$(call setStack,$(STACK_SIZE),$(NAME))

$(LIB_DRIVER):
	cd $(DRIVER_DIR) && $(MAKE)

clean:
	$(call cleanCommon)
	$(call cleanStack,$(NAME))

disasm: $(DEBUG)
	$(call disasmCode,$<,32)

