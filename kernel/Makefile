MY_HOME = ..
include $(MY_HOME)/common.mk
LIB_DIR = $(MY_HOME)/lib/m16
LIBS = -lmyc -lmysysutil
CFLAGS += -fno-asynchronous-unwind-tables

all: kernel

kernel: entry.o kernel.o 
	$(LD) $^ -Ttext 0x0 -m elf_i386 -nostdlib -L$(LIB_DIR) $(LIBS) -o $@ 

clean:
	rm -f *.o kernel
