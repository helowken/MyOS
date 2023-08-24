MY_HOME = ..
INC_HEADERS = *.h 
HEADERS = $(wildcard $(INC_HEADERS))
CFLAGS = -g -c -m32 -Wall -Werror -DOTHER_OS
COMMON_OBJS = util.o error_functions.o
MINIX_DIR = minix

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) $< -o $@

all: installboot.bin \
	 mkfs.bin \
	 partition.bin \
	 setstack.bin \
	 partsize.bin \
	 minix_mkfs


installboot.bin: installboot.o rawfs32.o $(COMMON_OBJS)
	$(CC) -g -m32 -o $@ $^ 

mkfs.bin: mkfs.o $(COMMON_OBJS) 
	$(CC) -g -m32 -o $@ $^ 

partition.bin: partition.o $(COMMON_OBJS)
	$(CC) -g -m32 -o $@ $^ 

setstack.bin: setstack.o $(COMMON_OBJS)
	$(CC) -g -m32 -o $@ $^ 

partsize.bin: partsize.o $(COMMON_OBJS)
	$(CC) -g -m32 -o $@ $^ 

minix_mkfs: 
	cd $(MINIX_DIR) && $(MAKE)


.PHONY:	clean 

clean:
	cd $(MINIX_DIR) && $(MAKE) clean
	rm -f *.o *.bin

