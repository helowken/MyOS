#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

static int lFlag;		/* Count lines */
static int wFlag;		/* Count words */
static int cFlag;		/* Count characters */

static long lCount;		/* Count of lines */
static long wCount;		/* Count of words */
static long cCount;		/* Count of characters */

static long lTotal;		/* Total count of lines */
static long wTotal;		/* Total count of words */
static long cTotal;		/* Total count of characters */

static void printCount() {
	if (lFlag) printf(" %6ld", lCount);
	if (wFlag) printf(" %6ld", wCount);
	if (cFlag) printf(" %6ld", cCount);
}

static void count(FILE *f) {
	register int c;
	register int word = 0;

	lCount = wCount = cCount = 0L;

	while ((c = getc(f)) != EOF) {
		++cCount;

		if (isspace(c)) {
			if (word)
			  ++wCount;
			word = 0;
		} else {
			word = 1;
		}
		if (c == '\n' || c == '\f')		/* '\f' is form feed */
		  ++lCount;
	}
	lTotal += lCount;
	wTotal += wCount;
	cTotal += cCount;
}

static void usage() {
	fprintf(stderr, "Usage: wc [-lwc] [name ...]\n");
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
	int k;
	char opt;
	int tFlag, files;
	FILE *f;

	/* Get flags. */
	while ((opt = getopt(argc, argv, "lwc")) != -1) {
		switch (opt) {
			case 'l': ++lFlag; break;
			case 'w': ++wFlag; break;
			case 'c': ++cFlag; break;
			default:  usage();
		}
	}
	k = optind;

	/* If no flags are set, treat as wc -lwc. */

	if (! lFlag && ! wFlag && ! cFlag) 
	  lFlag = wFlag = cFlag = 1;

	/* Check to see if input comes from std input. */
	if (k >= argc) {
		count(stdin);
		printCount();
		printf(" \n");
		fflush(stdout);
		exit(EXIT_SUCCESS);
	}

	/* Process files. */
	files = argc - k;
	tFlag = (files >= 2);	/* Set if # files > 1 */

	/* There is an explicit list of files. Loop on files. */
	while (k < argc) {
		if ((f = fopen(argv[k], "r")) == NULL) {
			fprintf(stderr, "wc: cannot open %s\n", argv[k]);
		} else {
			count(f);
			printCount();
			printf(" %s\n", argv[k]);
			fclose(f);
		}
		++k;
	}
	
	if (tFlag) {
		if (lFlag) printf(" %6ld", lTotal);
		if (wFlag) printf(" %6ld", wTotal);
		if (cFlag) printf(" %6ld", cTotal);
		printf(" total\n");
	}
	fflush(stdout);
	return 0;
}
