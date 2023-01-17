NAME = at_wini
PCI_DIR = ../libpci
LIB_PCI = $(PCI_DIR)/pci.o $(PCI_DIR)/pci_table.o 
OBJS = $(NAME).o $(LIB_PCI)
STACK_SIZE = 8k
include ../driver.mk

$(LIB_PCI): 
	cd $(PCI_DIR) && $(MAKE)
