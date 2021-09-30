export ARFLAGS = rvU

INC = $(MY_HOME)/include
INC_SYS = $(INC)/sys
INCLUDE = -I$(INC)
CFLAGS = -c -m32 -ffreestanding -nostdinc -nodefaultlibs -Wall -Werror $(INCLUDE)
ifeq ($(MODE), M16)
	CFLAGS += -D_M16
endif
ifdef DEBUG
	CFLAGS += -DDEBUG=1
else
	CFLAGS += -DDEBUG=0
endif

C_SOURCE = $(wildcard *.c)
HEADERS = $(wildcard *.h $(INC)/*.h $(INC_SYS)/*.h)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) $< -o $@

%.o: %.s
	$(AS) --32 $< -o $@ 

%.o: %.S
	$(CC) -c -Wa,--32 $(INCLUDE) $< -o $@

.PHONY:	clean 

