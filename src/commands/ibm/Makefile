MK = $(MAKE) -f ../bin.mk
MAKEFLAGS += --no-print-directory

all: 
	$(MK) NAME=loadkeys STACK_SIZE=4kw

clean: 
	$(MK) NAME=loadkeys clean

disasm: 
	$(MK) NAME=$(NAME) disasm
