MY_HOME = ../../..
CFLAGS = -g -m32 -Wall -Werror
BIN = tmp.bin

all: us-std.map

%.map : %.src genmap.c
	$(CC) $(CFLAGS) -DKEY_SRC=\"$<\" genmap.c -o $(BIN)
	./$(BIN) > $@
	@rm -f $(BIN)

.PHONY:	clean 

clean:
	rm -f *.map 
