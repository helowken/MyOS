export ARFLAGS = rvU

INC = $(MY_HOME)/include
INC_HEADERS = *.h \
			  $(INC)/*.h \
			  $(INC)/sys/*.h \
			  $(INC)/ibm/*.h \
			  $(INC)/minix/*.h 

CFLAGS = -g -c -m32 -ffreestanding -nostdinc -nodefaultlibs -Wall -Werror \
		 -fno-asynchronous-unwind-tables -I$(INC)
C_SOURCE = $(wildcard *.c)
HEADERS = $(wildcard $(INC_HEADERS))
DEBUG = debug.bin
ENTRY = $(MY_HOME)/entry/entry.o
LINKER_SCRIPT = $(MY_HOME)/linkerScript.lds
SETSTACK = $(MY_HOME)/tools/setstack.bin


%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) $< -o $@

%.o: %.s
	$(AS) -g --32 $< -o $@ 

%.o: %.S
	$(CC) -g -c -Wa,--32 -I$(INC) $< -o $@

.PHONY:	clean 


define link
	$(LD) $(1) -Ttext 0x0 -m elf_i386 -nostdlib $(2) -o $(3) -T $(LINKER_SCRIPT)
endef


define createDebug
	cp $(1) $(2)
	strip --strip-debug --strip-unneeded $(1)
endef


define setStack
	$(SETSTACK) $(1) $(2)
endef


define disasmCode
	objdump -S -d -j .text -j .rodata -j .data -j .bss -mi386 -Matt,data$(2),addr$(2) $(1) > /tmp/$(1).txt && vim -R /tmp/$(1).txt
endef


define cleanCommon
	rm -f *.o *.bin *.elf
endef



