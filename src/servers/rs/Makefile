OBJS = rs.o manager.o
NAME = rs
STACK_SIZE = 32kw
UTIL = service
UTIL_BIN = $(UTIL).bin
UTIL_DEBUG = debug_$(UTIL).bin
DST_DIR = $(RES_DIR)/bin

all: build $(UTIL_BIN)

include ../server.mk

$(UTIL_BIN): $(CRT_ENTRY) $(UTIL).o
	$(call mkbin,$^,$@,$(STACK_SIZE),$(UTIL_DEBUG),$(DST_DIR))

disasm_service: $(UTIL_DEBUG)
	$(call disasmCode,$<,32)

