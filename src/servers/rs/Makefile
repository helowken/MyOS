OBJS = rs.o manager.o
NAME = rs
STACK_SIZE = 32kw
UTIL = service
UTIL_BIN = $(UTIL).bin
DST_DIR = $(RES_DIR)/bin

all: build $(UTIL_BIN)

include ../server.mk

$(UTIL_BIN): $(CRT_ENTRY) $(UTIL).o
	$(call mkbin,$^,$@,$(STACK_SIZE),$(DST_DIR))

