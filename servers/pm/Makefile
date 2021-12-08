MY_HOME = ../..
INC_DEPEND = $(INC)/ibm/*.h $(INC)/minix/*.h
include $(MY_HOME)/common.mk
LIBS = -L$(MY_HOME)/lib \
	   -lmysys -lmysysutil -lmytimers -lmyc 

OBJS = main.o table.o utility.o
SERVER = pm.bin

all: $(SERVER)

$(SERVER): $(OBJS) 
	$(call link,$^,$(LIBS),$@) 
	$(call createDebug,$@,$(DEBUG))

clean:
	$(call cleanCommon)

disasm: $(DEBUG)
	$(call disasmCode,$<,32)

