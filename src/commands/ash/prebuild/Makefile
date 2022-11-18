CFLAGS = -g -m32 -Wall -Werror

MK_INIT = mkinit.bin
MK_SYNTAX = mksyntax.bin
MK_SIGNAMES = mksignames.bin
MK_NODES = mknodes.bin

all: $(MK_INIT) $(MK_SYNTAX) $(MK_SIGNAMES) $(MK_NODES)

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

$(MK_NODES): mknodes.c
	$(CC) $(CFLAGS) $^ -o $@

mknodes: $(MK_NODES)
	./$< nodetypes nodespat.c

clean: 
	rm -f *.o *.bin

