MY_HOME = ..
INC_HEADERS = *.h 
HEADERS = $(wildcard $(INC_HEADERS))
CFLAGS = -g -c -m32 -Wall -Werror 

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) $< -o $@

all: installboot.bin \
	 mkfs.bin

installboot.bin: installboot.o error_functions.o
	$(CC) -m32 -o $@ $^ 

mkfs.bin: mkfs.o error_functions.o
	$(CC) -m32 -o $@ $^ 

.PHONY:	clean 

clean:
	rm -f *.o *.bin

