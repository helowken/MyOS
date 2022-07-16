MY_HOME = ..
INC_HEADERS = *.h 
HEADERS = $(wildcard $(INC_HEADERS))
CFLAGS = -g -c -m32 -Wall -Werror -DOTHER_OS
COMMON_OBJS = util.o error_functions.o

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) $< -o $@

all: installboot.bin \
	 mkfs.bin \
	 partition.bin

installboot.bin: installboot.o rawfs32.o $(COMMON_OBJS)
	$(CC) -g -m32 -o $@ $^ 

mkfs.bin: mkfs.o $(COMMON_OBJS) 
	$(CC) -g -m32 -o $@ $^ 

partition.bin: partition.o $(COMMON_OBJS)
	$(CC) -g -m32 -o $@ $^ 


.PHONY:	clean 

clean:
	rm -f *.o *.bin

test: partition.bin
	./$< ../boot/device/testOS.img 1 81:32768* 81:40960 81:0+


