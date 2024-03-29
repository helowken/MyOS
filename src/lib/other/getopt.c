#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define ERR(s, c)	if (opterr) {\
	fputs(argv[0], stderr);\
	fputs(s, stderr);\
	fputc(c, stderr);\
	fputc('\n', stderr);}

int opterr = 1;
int optind = 1;
int optopt;
char *optarg;

int getopt(int argc, char * const argv[], const char *opts) {
	static int sp = 1;		/* 0 is '-' */
	register char c;
	register char *cp;

	if (sp == 1) {
		if (optind >= argc ||
				argv[optind][0] != '-' ||
				argv[optind][1] == '\0') {
			return EOF;
		} else if (strcmp(argv[optind], "--") == 0) {
			++optind;
			return EOF;
		}
	}
	optopt = c = argv[optind][sp];
	if (c == ':' || (cp = strchr(opts, c)) == NULL) {
		ERR(": illegal optioin -- ", c);
		if (argv[optind][++sp] == '\0') {
			++optind;
			sp = 1;
		}
		return '?';
	}
	if (*++cp == ':') {						/* opts = "i:" */
		if (argv[optind][sp + 1] != '\0')	/* argv: -ixxx */	
		  optarg = &argv[optind++][sp + 1];	/* optarg = xxx */
		else if (++optind >= argc) {
			ERR(": option requires an argument -- ", c);
			sp = 1;
			return '?';
		} else								/* argv: -i xxx */
		  optarg = argv[optind++];			/* optarg = xxx */
		sp = 1;
	} else {
		if (argv[optind][++sp] == '\0') {	/* argv: -i */
			sp = 1;
			++optind;
		}									/* else argv: -icm */
		optarg = NULL;
	}
	return c;
}
