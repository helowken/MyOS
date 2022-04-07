MY_HOME = ../..
include $(MY_HOME)/common.mk

LIBS = -L$(MY_HOME)/lib \
	   -lmysysutil -lmysys -lmytimers -lmyc 

all: $(SERVER)

$(SERVER): $(OBJS) 
	$(call link,$^,$(LIBS),$@) 
	$(call createDebug,$@,$(DEBUG))

clean:
	$(call cleanCommon)

disasm: $(DEBUG)
	$(call disasmCode,$<,32)

