CFLAGS = -g -m32 -Wall -Werror

MK_INIT = mkinit.bin
MK_SYNTAX = mksyntax.bin
MK_SIGNAMES = mksignames.bin

all: $(MK_INIT) $(MK_SYNTAX) $(MK_SIGNAMES)

$(MK_INIT): mkinit.c
	$(CC) $(CFLAGS) $^ -o $@

mkinit: $(MK_INIT) 
	./$< $(ARGS)

$(MK_SYNTAX): mksyntax.c ../parser.h
	$(CC) $(CFLAGS) $^ -o $@

mksyntax: $(MK_SYNTAX)
	./$<

$(MK_SIGNAMES): mksignames.c
	$(CC) $(CFLAGS) $^ -o $@

mksignames: $(MK_SIGNAMES)
	./$<

mktokens: mktokens.sh
	./$<

clean: 
	rm -f *.o *.bin

