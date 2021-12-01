export ARFLAGS = rvU

INC = $(MY_HOME)/include
CFLAGS = -c -m32 -ffreestanding -nostdinc -nodefaultlibs -Wall -Werror -I$(INC)
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

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) $< -o $@

%.o: %.s
	$(AS) -g --32 $< -o $@ 

%.o: %.S
	$(CC) -g -c -Wa,--32 -I$(INC) $< -o $@

.PHONY:	clean 

