#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define DEFAULT	10

static void doFile(int n, FILE *f) {
	int c;

	/* Print the first 'n' lines of a file. */
	while (n) {
		switch (c = getc(f)) {
			case EOF:
				return;
			case '\n':
				--n;
			default:
				putc((char) c, stdout);
		}
	}
}

static void usage() {
	fprintf(stderr, "Usage: head [-n] [file ...]\n");
	exit(1);
}

int main(int argc, char **argv) {
	FILE *f;
	int n, k, nFiles;
	char *p;

	/* Check for flag. Only flag is -n, to say how many lines to print. */
	k = 1;
	p = argv[1];
	n = DEFAULT;
	if (argc > 1 && *p++ == '-') {
		++k;
		if (*p == 'n') {
			++p;
			if (*p == '\0') {
				if (argc < 2)
				  usage();
				p = argv[2];
				++k;
			}
		}
		n = atoi(p);
		if (n <= 0) 
		  usage();
	}
	nFiles = argc - k;

	if (nFiles == 0) {
		/* Print standard input only. */
		doFile(n, stdin);
		return 0;
	}

	/* One or more files have been listed explicitly. */
	while (k < argc) {
		if (nFiles > 1)
		  printf("==> %s <==\n", argv[k]);
		if ((f = fopen(argv[k], "r")) == NULL)
		  fprintf(stderr, "%s: cannot open %s: %s\n",
				argv[0], argv[k], strerror(errno));
		else {
			doFile(n, f);
			fclose(f);
		}
		++k;
		if (k < argc)
		  printf("\n");
	}
	return 0;
}
