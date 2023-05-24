MY_HOME = ../..
include $(MY_HOME)/common.mk

LIBS = $(SYS_LIBS)
SERVER = $(NAME).bin
build: $(SERVER)

$(SERVER): $(ENTRY) $(OBJS) 
	$(call mkSysBin,$^,$@,$(STACK_SIZE))

clean:
	$(call cleanCommon)

disasm: $(DEBUG)
	$(call disasmCode,$<,32)

