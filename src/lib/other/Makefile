MY_HOME = ../..
LIB_NAME = myc
NEED_16 = 1

OBJS_M32 = \
		errno.o \
		syscall.o \
		settimeofday.o \
		getprocnr.o \
		findproc.o \
		allocmem.o \
		loadname.o \
		_svrctl.o \
		_brk.o \
		environ.o


OBJS_M16 = \
		$(DIR_16)/errno.o 


all: build

include ../commonlib.mk

build: $(LIB_M16) $(LIB_M32)

clean: clean16 clean32
