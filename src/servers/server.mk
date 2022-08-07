MY_HOME = ../..
include $(MY_HOME)/common.mk

LIBS = -L$(MY_HOME)/lib \
	   -lmysysutil -lmysys -lmytimers -lmyc 

SERVER = $(NAME).bin
all: $(SERVER)

$(SERVER): $(ENTRY) $(OBJS) 
	$(call link,$^,$(LIBS),$@) 
	$(call createDebug,$@,$(DEBUG))
	$(call setStack,$(STACK_SIZE),$(NAME))

clean:
	$(call cleanCommon)
	$(call cleanStack,$(NAME))

disasm: $(DEBUG)
	$(call disasmCode,$<,32)

