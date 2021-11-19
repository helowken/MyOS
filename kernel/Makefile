MY_HOME = ..
INC_DEPEND = $(INC)/ibm/*.h $(INC)/minix/*.h
include $(MY_HOME)/common.mk
CFLAGS += -g 
LIB_DIR = $(MY_HOME)/lib
LIBS = -lmyc -lmysysutil

HEAD = mpx.o
OBJS = start.o protect.o klib386.o table.o main.o proc.o \
	   i8259.o exception.o system.o clock.o utility.o
KERNEL = kernel.bin
DEBUG = debug.bin

all: $(KERNEL)

$(KERNEL): $(HEAD) $(OBJS)
	$(LD) $^ -Ttext 0x0 -m elf_i386 -nostdlib -L$(LIB_DIR) $(LIBS) -o $@ 
	cp $@ $(DEBUG)
	strip --strip-debug --strip-unneeded $@

test: entry.o test.o
	$(LD) $^ -Ttext 0x0 -m elf_i386 -nostdlib -o $(KERNEL)

disasm: $(DEBUG)
	objdump -S -d -j .text -j .data -j .bss -mi386 -Matt,data32,addr32 $< | less

clean:
	rm -f *.o *.bin
