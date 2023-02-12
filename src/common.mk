export ARFLAGS = rvU

INC = $(MY_HOME)/include
INC_HEADERS = *.h \
			  $(INC)/*.h \
			  $(INC)/sys/*.h \
			  $(INC)/ibm/*.h \
			  $(INC)/minix/*.h 

CFLAGS = -g -c -m32 \
		 -ffreestanding -nostdinc -nostartfiles -nodefaultlibs \
		 -fno-asynchronous-unwind-tables -I$(INC) \
		 -Wall -Werror 
		 
C_SOURCE = $(wildcard *.c)
HEADERS = $(wildcard $(INC_HEADERS))
SYS_LIBS = -L$(MY_HOME)/lib -lmysys -lmysysutil -lmytimers -lmyc
LIBS = $(SYS_LIBS)
DEBUG = debug.bin
ENTRY = $(MY_HOME)/entry/entry.o
CRT_ENTRY = $(MY_HOME)/entry/crtso.o
LINKER_SCRIPT = $(MY_HOME)/linkerScript.lds
SETSTACK = $(MY_HOME)/tools/setstack.bin

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) $< -o $@

%.o: %.s
	$(AS) -g --32 $< -o $@ 

%.o: %.S
	$(CC) -g -c -Wa,--32 -I$(INC) $< -o $@

.PHONY:	clean 


# link(srcs, libs, dst)
define link
	$(LD) $(1) -Ttext 0x0 -m elf_i386 -nostdlib $(2) -o $(3) -T $(LINKER_SCRIPT)
endef


# createDebug(srcBin, debugBin)
define createDebug
	cp $(1) $(2)
	strip --strip-debug --strip-unneeded $(1)
endef


# setStack(bin, stackSize)
define setStack
	$(SETSTACK) $(1) $(2)
endef


# disasmCode()
define disasmCode
	objdump -S -d -j .text -j .rodata -j .data -j .bss -mi386 -Matt,data$(2),addr$(2) $(1) > /tmp/$(1).txt && vim -R /tmp/$(1).txt
endef


# cleanCommon()
define cleanCommon
	rm -f *.o *.bin *.elf
endef


# mkSysBin(srcs, bin, stackSize)
define mkSysBin 
	$(call link,$(1),$(LIBS),$(2)) 
	$(call createDebug,$(2),$(DEBUG))
	@$(call setStack,$(3),$(2))
endef


# mkbin(srcs, bin, stackSize, debugBin)
define mkbin 
	$(call link,$(1),$(LIBS),$(2)) 
	$(call createDebug,$(2),$(4))
	@$(call setStack,$(3),$(2))
endef

