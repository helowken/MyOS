/* Cmpare two files */

#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>


#define BLOCK	4096

static int loud = 0;	/* Print bytes that differ (in octal) */
static int silent = 0;	/* Print nothing, just return exit status */
static char *name1, *name2;

static void usage() {
	fprintf(stderr, "Usage: cmp [-l | -s] file1 file2\n");
	exit(2);
}

static void fatal(char *label) {
	if (! silent)
	  fprintf(stderr, "cmp: %s: %s\n", label, strerror(errno));
	exit(2);
}

static int cmp(int fd1, int fd2) {
	static char buf1[BLOCK], buf2[BLOCK];
	int n1 = 0, n2 = 0, i1 = 0, i2 = 0, c1, c2;
	off_t pos = 0, line = 1;
	int eof = 0, differ = 0;

	while (1) {
		if (i1 == n1) {
			pos += n1;

			if ((n1 = read(fd1, buf1, sizeof(buf1))) <= 0) {
				if (n1 < 0)
				  fatal(name1);
				eof |= 1;
			}
			i1 = 0;
		}
		if (i2 == n2) {
			if ((n2 = read(fd2, buf2, sizeof(buf2))) <= 0) {
				if (n2 < 0)
				  fatal(name2);
				eof |= 2;
			}
			i2 = 0;
		}
		if (eof != 0)
		  break;

		c1 = buf1[i1++];
		c2 = buf2[i2++];

		if (c1 != c2) {
			if (! loud) {
				if (! silent) 
				  printf("%s %s differ: char %ld, line %ld\n", 
						  name1, name2, pos + i1, line);
				return 1;
			}
			printf("%10ld %3o %3o\n", pos + i1, c1 & 0xFF, c2 & 0xFF);
			differ = 1;
		}
		if (c1 == '\n')
		  ++line;
	}
	if (eof == (1 | 2))
	  return differ;
	if (! silent)
	  fprintf(stderr, "cmp: EOF on %s\n", eof == 1 ? name1 : name2);
	return 1;
}

void main(int argc, char **argv) {
	int fd1, fd2;
	int opt;

	while ((opt = getopt(argc, argv, "ls")) != -1) {
		switch (opt) {
			case 'l':
				loud = 1;
				break;
			case 's':
				silent = 1;
				break;
			default:
				usage();
		}
	}

	if (loud && silent) 
	  usage();
	if (optind + 1 >= argc) 
	  usage();

	name1 = argv[optind++];
	if (name1[0] == '-' && name1[1] == '\0') {
		name1 = "stdin";
		fd1 = STDIN_FILENO;
	} else {
		if ((fd1 = open(name1, 0)) < 0)
		  fatal(name1);
	}

	name2 = argv[optind];
	if (name2[0] == '-' && name2[1] == '\0') {
		name2 = "stdin";
		fd2 = STDIN_FILENO;
	} else {
		if ((fd2 = open(name2, 0)) < 0)
		  fatal(name2);
	}

	exit(cmp(fd1, fd2));
}
