export ARFLAGS = rvU

INC = $(MY_HOME)/include
INC_SYS = $(INC)/sys
INCLUDE = -I$(INC) -I$(INC_SYS)
CFLAGS = -c -m32 -ffreestanding -Wall -Werror $(INCLUDE)
ifeq ($(MODE), M16)
	CFLAGS += -D_M16
endif

C_SOURCE = $(wildcard *.c)
HEADERS = $(wildcard *.h, $(INC)/*.h, $(INC_SYS)/*.h)


%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) $< -o $@

%.o: %.s
	$(AS) --32 $< -o $@
  


