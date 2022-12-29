MY_HOME = ../..
LIB_NAME = mysysutil
NEED_16 = 1

OBJS_M32 = \
		kprintf.o \
		kputc.o \
		report.o \
		panic.o \
		getuptime.o \
		tickdelay.o \
		env_get_param.o
	

OBJS_M16 = \
		$(DIR_16)/kprintf.o \
		$(DIR_16)/kmalloc.o


all: build

include ../commonlib.mk

build: $(LIB_M16) $(LIB_M32)

clean: clean16 clean32
