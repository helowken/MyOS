MK = $(MAKE) -f bin.mk

all: 
	$(MK) NAME=loadkeys STACK_SIZE=$(shell expr 4096 '*' 32)
	$(MK) NAME=testProc STACK_SIZE=1024

clean: 
	$(MK) NAME=loadkeys clean
	$(MK) NAME=testProc clean
