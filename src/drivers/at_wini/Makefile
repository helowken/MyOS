MY_HOME = ../..
include $(MY_HOME)/common.mk

PCI_DIR = ../libpci
DRIVER_DIR = ../libdriver

OBJS = $(ENTRY) at_wini.o 
LIB_PCI = $(PCI_DIR)/pci.o $(PCI_DIR)/pci_table.o 
LIB_DRIVER = $(DRIVER_DIR)/driver.o $(DRIVER_DIR)/drvlib.o
DRIVER = at_wini.bin
STACK_SIZE = 8k

all: $(DRIVER)

$(DRIVER): $(OBJS) $(LIB_PCI) $(LIB_DRIVER)
	$(call link,$^,$(LIBS),$@) 
	$(call createDebug,$@,$(DEBUG))
	$(call setStack,$(STACK_SIZE),$@)

$(LIB_PCI): 
	cd $(PCI_DIR) && $(MAKE)

$(LIB_DRIVER):
	cd $(DRIVER_DIR) && $(MAKE)

clean:
	$(call cleanCommon)

disasm: $(DEBUG)
	$(call disasmCode,$<,32)

