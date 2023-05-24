MY_HOME = ../..
include $(MY_HOME)/common.mk

LIBS = $(SYS_LIBS)
DRIVER_DIR = ../libdriver
LIB_DRIVER = $(DRIVER_DIR)/driver.o $(DRIVER_DIR)/drvlib.o
DRIVER = $(NAME).bin

all: $(DRIVER)

$(DRIVER): $(ENTRY) $(OBJS) $(LIB_DRIVER) $(OTHERS)
	$(call mkSysBin,$^,$@,$(STACK_SIZE))

$(LIB_DRIVER):
	cd $(DRIVER_DIR) && $(MAKE)

clean:
	$(call cleanCommon)

disasm: $(DEBUG)
	$(call disasmCode,$<,32)

