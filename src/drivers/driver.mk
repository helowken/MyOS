MY_HOME = ../..
include $(MY_HOME)/common.mk

DRIVER_DIR = ../libdriver
LIB_DRIVER = $(DRIVER_DIR)/driver.o $(DRIVER_DIR)/drvlib.o
DRIVER = $(NAME).bin
DEL_FILES = $(DRIVER)

all: $(DRIVER)

$(DRIVER): $(ENTRY) $(OBJS) $(LIB_DRIVER) $(OTHERS)
	$(call mkSysBin,$^,$@,$(STACK_SIZE))

$(LIB_DRIVER):
	cd $(DRIVER_DIR) && $(MAKE)

clean:
	$(call cleanCommon)
	rm -f $(DEL_FILES)

disasm: $(DEBUG)
	$(call disasmCode,$<,32)

