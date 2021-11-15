MY_HOME = ..
INC_DEPEND = $(INC)/ibm/*.h $(INC)/minix/*.h
include $(MY_HOME)/common.mk
CFLAGS += -fno-asynchronous-unwind-tables
LIB_DIR = $(MY_HOME)/lib
LIBS = -lmyc -lmysysutil

HEAD = mpx.o
OBJS = start.o protect.o klib386.o table.o main.o proc.o \
	   i8259.o exception.o system.o clock.o utility.o
KERNEL = kernel.bin

all: $(KERNEL)

$(KERNEL): $(HEAD) $(OBJS)
	$(LD) $^ -Ttext 0x0 -m elf_i386 -nostdlib -L$(LIB_DIR) $(LIBS) -o $@ 

test: entry.o test.o
	$(LD) $^ -Ttext 0x0 -m elf_i386 -nostdlib -o $(KERNEL)

disasm: $(KERNEL)
	objdump -d -j .text -j .data -j .bss -mi386 -Matt,data32,addr32 $< | less

clean:
	rm -f *.o *.bin
