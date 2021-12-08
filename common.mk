export ARFLAGS = rvU

INC = $(MY_HOME)/include
CFLAGS = -g -c -m32 -ffreestanding -nostdinc -nodefaultlibs -Wall -Werror -I$(INC)
ifeq ($(MODE), M16)
	CFLAGS += -D_M16
endif
ifdef DEBUG
	CFLAGS += -DDEBUG=1
else
	CFLAGS += -DDEBUG=0
endif

C_SOURCE = $(wildcard *.c)
HEADERS = $(wildcard *.h $(INC)/*.h $(INC)/sys/*.h $(INC_DEPEND))
DEBUG = debug.bin

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) $< -o $@

%.o: %.s
	$(AS) -g --32 $< -o $@ 

%.o: %.S
	$(CC) -g -c -Wa,--32 -I$(INC) $< -o $@

.PHONY:	clean 


define link
	$(LD) $(1) -Ttext 0x0 -m elf_i386 -nostdlib $(2) -o $(3)
endef


define createDebug
	cp $(1) $(2)
	strip --strip-debug --strip-unneeded $(1)
endef


define disasmCode
	objdump -S -d -j .text -j .data -j .bss -mi386 -Matt,data$(2),addr$(2) $(1) | less
endef


define cleanCommon
	rm -f *.o *.bin *.elf
endef
