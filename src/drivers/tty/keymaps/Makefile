MY_HOME = ../../..
CFLAGS = -g -m32 -Wall -Werror
RES_DIR = $(MY_HOME)/tools/resources
DST_DIR = $(RES_DIR)/etc
BIN = tmp.bin

all: us-std.map

%.map : %.src genmap.c
	$(CC) $(CFLAGS) -DKEY_SRC=\"$<\" genmap.c -o $(BIN)
	./$(BIN) > $@
	@rm -f $(BIN)
	mkdir -p $(DST_DIR)
	cp $@ $(DST_DIR)

.PHONY:	clean 

clean:
	rm -f *.map $(DST_DIR)/*.map
