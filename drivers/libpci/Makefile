MY_HOME = ../..
include $(MY_HOME)/common.mk
LIBS = -L$(MY_HOME)/lib \
	   -lmysys -lmysysutil -lmyc 

OBJS = pci.o pci_table.o 

all: $(OBJS)

clean:
	$(call cleanCommon)


