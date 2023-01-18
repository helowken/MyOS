MK = $(MAKE) -f ../bin.mk
MAKEFLAGS += --no-print-directory

all: 
	$(MK) NAME=sysenv STACK_SIZE=4kw
	$(MK) NAME=testAAA STACK_SIZE=4kw

clean: 
	$(MK) NAME=sysenv clean
	$(MK) NAME=testAAA clean

disasm: 
	$(MK) NAME=$(NAME) disasm
