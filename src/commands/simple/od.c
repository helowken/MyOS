#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

static int bFlag, cFlag, dFlag, oFlag, xFlag, hFlag, vFlag;
static int lineNum, width, state, ever;
static int preWords[8];
static long off;
static char buf[512], buffer[BUFSIZ];
static int next;
static int bytesPresent;

static void usage() {
	fprintf(stderr, "Usage: od [-bcdhovx] [file] [[+]offset[.]] [b]\n");
	exit(1);
}

static long offset(int argc, char **argv, int k) {
	int dot, radix;
	char *p, c;
	long val;

	/* See if the offset is decimal. */
	dot = 0;
	p = argv[k];
	while (*p) {
		if (*p++ == '.') {
		  dot = 1;
		  break;
		}
	}

	/* Convert offset to binary. */
	radix = (dot ? 10 : 8);
	val = 0;
	p = argv[k];
	if (*p == '+')
	  ++p;
	while (*p != 0 && *p != '.') {
		c = *p++;
		if (c < '0' || c > '9') {
			fprintf(stderr, "Bad character in offset: %c\n", c);
			exit(1);
		}
		val = radix * val + (c - '0');
	}

	p = argv[k + 1];
	if (k + 1 == argc - 1 && *p == 'b')
	  val *= 512L;

	return val;
}

static int getWords(short **words) {
	int count;

	if (next >= bytesPresent) {
		bytesPresent = read(0, buf, 512);
		next = 0;
	}
	if (next >= bytesPresent)
	  return 0;

	*words = (short *) &buf[next];
	if (next + 16 <= bytesPresent)
	  count = 16;
	else
	  count = bytesPresent - next;

	next += count;
	return count;
}

static int same(short *w1, int *w2) {
	int i;

	i = 8;
	while (i--) {
		if (*w1++ != *w2++)
		  return 0;
	}
	return 1;
}

static void outNum(int num, int radix) {
/* Output a number with all leading 0s present. Octal is 6 places,
 * decimal is 5 places, hex is 4 places.
 */
	unsigned val;

	val = (unsigned) num;
	switch (radix) {
		case 7:  printf("%03o", val); break;	/* special case */
		case 8:  printf("%06o", val); break;
		case 10: printf("%05u", val); break;
		case 16: printf("%04x", val); break;
	}
}

static void outWord(int val, int radix) {
/* Output 'val' in 'radix' in a field of total size 'width'. */
	int i;

	switch (radix) {
		case 8:  i = width - 6; break;
		case 10: i = width - 5; break;
		case 16: i = width - 4; break;
	}
	switch (i) {
		case 1:	printf(" "); break;
		case 2: printf("  "); break;
		case 3: printf("   "); break;
		case 4: printf("    "); break;
	}
	outNum(val, radix);
}

static void wordDump(short *words, int k, int radix) {
	int i;

	if (lineNum++ != 1)
	  printf("       ");
	for (i = 0; i < (k + 1) / 2; ++i) {
		outWord(words[i] & 0xFFFF, radix);
	}
	printf("\n");
}

static void outByte(int val, char c) {
	if (c == 'b') {
		printf(" ");
		outNum(val, 7);
		return;
	}
	switch (val) {
		case 0:    printf("  \\0"); break;
		case '\b': printf("  \\b"); break;
		case '\f': printf("  \\f"); break;
		case '\n': printf("  \\n"); break;
		case '\r': printf("  \\r"); break;
		case '\t': printf("  \\t"); break;
		default:
			if (val >= ' ' && val < 0177)
			  printf("   %c", val);
			else {
				printf(" ");
				outNum(val, 7);
			}
	}
}

static void byteDump(char bytes[16], int k, char c) {
	int i;

	if (lineNum++ != 1)
	  printf("       ");
	for (i = 0; i < k; ++i) {
		outByte(bytes[i] & 0377, c);
	}
	printf("\n");
}

static void addrOut(long addr) {
	if (hFlag == 0) 
	  printf("%07lo", addr);
	else
	  printf("%07lx", addr);
}

static void dumpFile() {
	int k;
	short *words;

	while ((k = getWords(&words))) {	/* 'k' is # bytes read */
		if (! vFlag) {	/* Ensure 'lazy' evaluation */
			if (k == 16 && ever == 1 && same(words, preWords)) {
				if (state == 0) {
					printf("*\n");
					state = 1;
					off += 16;
					continue;
				} else if (state == 1) {
					off += 16;
					continue;
				}
			}
		}
		addrOut(off);
		off += k;
		state = 0;
		ever = 1;
		lineNum = 1;
		if (oFlag) wordDump(words, k, 8);
		if (dFlag) wordDump(words, k, 10);
		if (xFlag) wordDump(words, k, 16);
		if (cFlag) byteDump((char *) words, k, (int) 'c');
		if (bFlag) byteDump((char *) words, k, (int) 'b');
		for (k = 0; k < 8; ++k) {
			preWords[k] = words[k];
		}
		for (k = 0; k < 8; ++k) {
			words[k] = 0;
		}
	}
}

int main(int argc, char **argv) {
	int k, flags;
	char *p;

	/* Process flags */
	setbuf(stdout, buffer);
	flags = 0;
	p = argv[1];
	if (argc > 1 && *p == '-') {
		/* Flags present. */
		++flags;
		++p;
		while (*p) {
			switch (*p) {
				case 'b': ++bFlag; break;
				case 'c': ++cFlag; break;
				case 'd': ++dFlag; break;
				case 'h': ++hFlag; break;
				case 'o': ++oFlag; break;
				case 'v': ++vFlag; break;
				case 'x': ++xFlag; break;
				default: usage();
			}
			++p;
		}
	} else {
		oFlag = 1;
	}
	if ((bFlag | cFlag | dFlag | oFlag | xFlag) == 0)
	  oFlag = 1;

	k = (flags ? 2 : 1);
	if (bFlag | cFlag) 
	  width = 8;
	else if (oFlag)
	  width = 7;
	else if (dFlag)
	  width = 6;
	else
	  width = 5;

	/* Process file name, if any. */
	p = argv[k];
	if (k < argc && *p != '+') {
		/* Explicit file name given. */
		close(STDIN_FILENO);
		if (open(argv[k], O_RDONLY) != 0) {
			fprintf(stderr, "od: cannot open %s\n", argv[k]);
			exit(1);
		}
		++k;
	}

	/* Process offset, if any. */
	if (k < argc) {
		/* Offset present. */
		off = offset(argc, argv, k);
		off = off / 16L * 16L;
		lseek(STDIN_FILENO, off, SEEK_SET);
	}
	dumpFile();
	addrOut(off);
	printf("\n");
	return 0;
}




