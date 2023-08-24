#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define BUFFER_SIZE	1024
#define ASCII		0377

typedef char BOOL;
#define TRUE	1
#define FALSE	0

#define NIL_PTR		((char *) 0)

static BOOL comFlag, delFlag, sqFlag;
static unsigned char output[BUFFER_SIZE], input[BUFFER_SIZE];
static unsigned char vector[ASCII + 1];
static BOOL inVec[ASCII + 1], outVec[ASCII + 1];
static short inIdx, outIdx;


static void expand(register char *arg, register unsigned char *buffer) {
	int i, ac;

	while (*arg) {
		if (*arg == '\\') {		
			++arg;	
			i = ac = 0;	
			if (*arg >= '0' && *arg <= '7') {	/* Octal */
				do {
					ac = (ac << 3) + (*arg++ - '0');
					++i;
				} while (i < 4 && *arg >= '0' && *arg <= '7');
				*buffer++ = ac;
			} else if (*arg != '\0') {
				*buffer++ = *arg++;
			}
		} else if (*arg == '[') {	/* May be [start-end] */
			++arg;
			i = *arg++;		/* start */
			if (*arg++ != '-') {	/* Not [start-end] */
				*buffer++ = '[';
				arg -= 2;
				continue;
			}
			ac = *arg++;	/* end */
			while (i <= ac) {
				*buffer++ = i++;
			}
			++arg;	/* Skip ']' */
		} else {
			*buffer++ = *arg++;
		}
	}
}

static void complement(unsigned char *buffer) {
	register unsigned char *ptr;
	register short i, index;
	unsigned char conv[ASCII + 2];

	index = 0;
	for (i = 1; i <= ASCII; ++i) {
		for (ptr = buffer; *ptr; ++ptr) {
			if (*ptr == i)
			  break;
		}
		if (*ptr == '\0')
		  conv[index++] = i & ASCII;
	}
	conv[index] = '\0';
	strcpy((char *) buffer, (char *) conv);
}

static void map(register unsigned char *string1, register unsigned char *string2) {
	unsigned char last;

	while (*string1) {
		if (*string2 == '\0') 
		  vector[*string1] = last;
		else
		  vector[*string1] = last = *string2++;
		++string1;
	}
}

static void convert() {
	short readChars = 0;
	short c, coded;
	short last = -1;

	for (;;) {
		if (inIdx == readChars) {
			if ((readChars = read(STDIN_FILENO, (char *) input, BUFFER_SIZE)) <= 0) {
				if (write(STDOUT_FILENO, (char *) output, outIdx) != outIdx) 
				  fprintf(stderr, "Bad write\n");
				exit(EXIT_SUCCESS);
			}
			inIdx = 0;
		}
		c = input[inIdx++];
		coded = vector[c];
		if (delFlag && inVec[c])
		  continue;
		if (sqFlag && last == coded && outVec[coded])
		  continue;
		output[outIdx++] = last = coded;
		if (outIdx == BUFFER_SIZE) {
			if (write(STDOUT_FILENO, (char *) output, outIdx) != outIdx) {
				fprintf(stderr, "Bad write\n");
				exit(EXIT_FAILURE);
			}
			outIdx = 0;
		}
	}
}

void main(int argc, char **argv) {
	register unsigned char *ptr;
	short i;
	int opt;

	while ((opt = getopt(argc, argv, "cds")) != EOF) {
		switch (opt) {
			case 'c':	comFlag = TRUE; break;
			case 'd':	delFlag = TRUE; break;
			case 's':	sqFlag = TRUE;  break;
			default:
				fprintf(stderr, "Usage: tr [-cds] [string1 [string2]].\n");
				exit(1);
		}
	}

	for (i = 0; i <= ASCII; ++i) {
		vector[i] = i;
		inVec[i] = outVec[i] = FALSE;
	}

	if (optind < argc) {
		expand(argv[optind++], input);
		if (comFlag)
		  complement(input);
		if (optind < argc) {
		  expand(argv[optind], output);
		  map(input, output);
		}
		for (ptr = input; *ptr; ++ptr) {
			inVec[*ptr] = TRUE;
		}
		for (ptr = output; *ptr; ++ptr) {
			outVec[*ptr] = TRUE;
		}
	}
	convert();
}
