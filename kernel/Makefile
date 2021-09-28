MY_HOME = ..
include $(MY_HOME)/common.mk
CFLAGS += -fno-asynchronous-unwind-tables

all: kernel

kernel: entry.o kernel.o 
	$(LD) $^ -Ttext 0x0 -m elf_i386 -nostdlib -o $@ 

clean:
	rm -f *.o kernel
