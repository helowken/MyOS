MY_HOME = ../..
include $(MY_HOME)/common.mk
LIBS = -L$(MY_HOME)/lib \
	   -lmysys -lmysysutil -lmytimers -lmyc 

OBJS = entry.o main.o table.o utility.o alloc.o timers.o signal.o \
	   forkexit.o trace.o exec.o time.o misc.o getset.o
SERVER = pm.bin

all: $(SERVER)

$(SERVER): $(OBJS) 
	$(call link,$^,$(LIBS),$@) 
	$(call createDebug,$@,$(DEBUG))

clean:
	$(call cleanCommon)

disasm: $(DEBUG)
	$(call disasmCode,$<,32)

