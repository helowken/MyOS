/* grep - search a file for a pattern */

/* Grep searches files for lines containing a pattern, as specified by
 * a regular expression, and prints those lines. It is invoked by:
 *	grep [flags] [pattern] [file ...]
 *
 * Flags:
 *	-e pattern	useful when pattern begins with '-'
 *	-c		print a count of lines matched
 *	-i		ignore case
 *	-l		prints just file names, no lines (quietly overrides -n)
 *	-n		printed lines are preceded by relative line numbers
 *	-s		prints errors only (quietly overrides -l and -n)
 *	-v		prints lines which don't contain the pattern
 *
 * Semantic note:
 *	If both -l and -v are specified, grep prints the names of those
 *	files which do not contain the pattern *anywhere*.
 *
 * Exit:
 *	Grep sets an exit status which can be tested by the caller.
 *	Note that these settings are not necessarily compatible with
 *	any other version of grep, especially when -v is specified.
 *	Possible status values are:
 *		0 if any matches are found
 *		1 if no matches are found
 *		2 if syntax errors are detected or any file cannot be opened
 */

#include <sys/types.h>
#include <regexp.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#define MATCH		0	/* Exit code: some match somewhere */
#define NO_MATCH	1	/* Exit code: no match on nay line */
#define FAILURE		2	/* Exit code: syntax error or bad file name */

#define SET_FLAG(c)	(flags[(c) - 'a'] = 1)
#define FLAG(c)		(flags[(c) - 'a'] != 0)

#define isUppercase(c)	(((unsigned) ((c) - 'A')) <= ('Z' - 'A'))
#define downcase(c)		((c) - 'A' + 'a')

static char *progName;
static char flags[26];
static regexp *expression;
static char *errMsg;


/* getLine - fetch a line from the input file.
 * This function reads a line from the input file into a dynamically
 * allocated buffer. If the line is too long for the current buffer,
 * attempts will be made to increase its size to accomodate the line.
 * The trailing newline is stripped before returning to the caller.
 */
#define FIRST_BUFFER	(size_t) 256	/* First buffer size */
static char *buf = NULL;	/* Input buffer */
static size_t bufSize = 0;	/* Input buffer size */



static char *getLine(FILE *input) {
	int n;
	register char *bp;
	register int c;
	char *newBuf;
	size_t newSize;

	if (bufSize == 0) {
		if ((buf = (char *) malloc(FIRST_BUFFER)) == NULL) {
			fprintf(stderr, "%s: not enough memory\n", progName);
			exit(FAILURE);
		}
		bufSize = FIRST_BUFFER;
	}

	bp = buf;
	n = bufSize;
	while (1) {
		while (--n > 0 && (c = getc(input)) != EOF) {
			if (c == '\n') {
				*bp = '\0';
				return buf;
			}
			*bp++ = c;
		}
		if (c == EOF)
		  return ferror(input) || bp == buf ? NULL : buf;
		
		newSize = bufSize << 1;
		if ((newBuf = (char *) realloc(buf, newSize)) == NULL) {
			fprintf(stderr, "%s: lien too long - truncated\n", progName);
			while ((c = getc(input)) != EOF && c != '\n') {
			}
			*bp = '\0';
			return buf;
		} else {
			bp = newBuf + (bufSize - 1);
			n = bufSize + 1;
			buf = newBuf;
			bufSize = newSize;
		}
	}	
}

/* mapNoCase - map a line down to lowercase letters only.
 * bad points: assumes line gotten from getLine.
 *		there is more than A-Z you say?
 */
static char *mapNoCase(char *line) {
	static char *mapped = NULL;
	static size_t mapSize = 0;
	char *mp;

	if (mapSize < bufSize) {
		mapSize = bufSize;
		if ((mapped = realloc(mapped, mapSize)) == NULL) {
			fprintf(stderr, "%s: not enough memory\n", progName);
			exit(FAILURE);
		}
	}

	mp = mapped;
	do {
		*mp++ = isUppercase(*line) ? downcase(*line) : *line;
	} while (*line++ != 0);

	return mapped;
}

/* match - matches the lines of a file with the regular expression.
 * To imporve performance when either -s or -l is specified, this
 * function handles those cases specially.
 */
static int match(FILE *input, char *label, char *fileName) {
	char *line, *testLine;	/* Pointers to input line */
	long int lineNum = 0;	
	long int matchCount = 0;
	int status = NO_MATCH;	/* Summary of what was found in this file. */

	if (FLAG('s') || FLAG('l')) {
		while ((line = getLine(input)) != NULL) {
			testLine = FLAG('i') ? mapNoCase(line) : line;
			if (regexec(expression, testLine, 1)) {
				status = MATCH;
				break;
			}
		}
		if (FLAG('l')) {
			if ((! FLAG('v') && status == MATCH) ||
					(FLAG('v') && status == NO_MATCH))
			  puts(fileName);
		}
		return status;
	}

	while ((line = getLine(input)) != NULL) {
		++lineNum;
		testLine = FLAG('i') ? mapNoCase(line) : line;
		if (regexec(expression, testLine, 1)) {
			status = MATCH;
			if (! FLAG('v')) {
				if (label != NULL)
				  printf("%s:", label);
				if (FLAG('n')) 
				  printf("%ld:", lineNum);
				if (! FLAG('c'))
				  puts(line);
				++matchCount;
			}
		} else {
			if (FLAG('v')) {
				if (label != NULL)
				  printf("%s:", label);
				if (FLAG('n')) 
				  printf("%ld:", lineNum);
				if (! FLAG('c'))
				  puts(line);
				++matchCount;
			}
		}
	}
	if (FLAG('c'))
	  printf("%ld\n", matchCount);
	return status;
}

/* In basic regular expressions, the characters ?, +, |, (, and )
 * are taken literally; use the backslashed versions for RE operators.
 * In v8 regular expressions, things are the other way round, so
 * we have to swap those characters and their backslashed versions.
 */
static void toV8(char *v8, char *basic) {
	while (*basic) {
		switch (*basic) {
			case '?':
			case '+':
			case '|':
			case '(':
			case ')':
				*v8++ = '\\';
				*v8++ = *basic++;
				break;
			case '\\':
				switch (*(basic + 1)) {
					case '?':
					case '+':
					case '|':
					case '(':
					case ')':
						*v8++ = *++basic;
						++basic;
						break;
					case '\0':
						*v8++ = *basic++;
						break;
					default:
						*v8++ = *basic++;
						*v8++ = *basic++;
				}
				break;
			default:
				*v8++ = *basic++;
		}
	}
	*v8++ = '\0';
}

int main(int argc, char **argv) {
	int opt;			/* Option letter from getopt() */
	int egrep = 0;		/* Using extended regexp operators */
	char *pattern;		/* Search pattern */
	char *v8pattern;	/* V8 regexp search pattern */
	int exitStatus = NO_MATCH;	
	int fileStatus;		/* Status of search in one file */
	FILE *input;		/* Input file (if not stdin) */
	
	progName = argv[0];
	if (strlen(progName) >= 5 && 
				strcmp(progName + strlen(progName) - 5, "egrep") == 0)
	  egrep = 1;
	
	memset(flags, 0, sizeof(flags));
	pattern = NULL;

	/* Process any command line flags. */
	while ((opt = getopt(argc, argv, "e:cilnsv")) != EOF) {
		if (opt == '?')
		  exitStatus = FAILURE;
		else if (opt == 'e')
		  pattern = optarg;
		else
		  SET_FLAG(opt);
	}

	/* Detect a few problems. */
	if (exitStatus == FAILURE || (optind == argc && pattern == NULL)) {
		fprintf(stderr, "Usage: %s [-cilnsv] [-e] expression [file ...]\n", progName);
		exit(FAILURE);
	}

	/* Ensure we have a usable pattern. */
	if (pattern == NULL)
	  pattern = argv[optind++];

	/* Map pattern to lowercase if -i given. */
	if (FLAG('i')) {
		char *p;
		for (p = pattern; *p != '\0'; ++p) {
			if (isUppercase(*p))
			  *p = downcase(*p);
		}
	}

	if (! egrep) {
		if ((v8pattern = malloc(2 * strlen(pattern))) == NULL) {
			fprintf(stderr, "%s: out of memory\n");
			exit(FAILURE);
		}
		toV8(v8pattern, pattern);
	} else 
	  v8pattern = pattern;

	errMsg = NULL;
	if ((expression = regcomp(v8pattern)) == NULL) {
		fprintf(stderr, "%s: bad regular expression", progName);
		if (errMsg)		/* set by regerror() */
		  fprintf(stderr, " (%s)", errMsg);
		fprintf(stderr, "\n");
		exit(FAILURE);
	}

	/* Process the files appropriately. */
	if (optind == argc) {	/* No file names - find pattern in stdin */
		exitStatus = match(stdin, NULL, "<stdin>");
	} else if (optind + 1 == argc) {	/* one file name - find pattern in it */
		if (strcmp(argv[optind], "-") == 0) {
			exitStatus = match(stdin, NULL, "-");
		} else {
			if ((input = fopen(argv[optind], "r")) == NULL) {
				fprintf(stderr, "%s: couldn't open %s\n", progName, argv[optind]);
				exitStatus = FAILURE;
			} else {
				exitStatus = match(input, NULL, argv[optind]);
			}
		} 
	} else {
		while (optind < argc) {	/* Lots of file names - find pattern in all */
			if (strcmp(argv[optind], "-") == 0) {
				fileStatus = match(stdin, "-", "-");
			} else {
				if ((input = fopen(argv[optind], "r")) == NULL) {
					fprintf(stderr, "%s: couldn't open %s\n", progName, argv[optind]);
					exitStatus = FAILURE;
				} else {
					fileStatus = match(input, argv[optind], argv[optind]);
					fclose(input);
				}
			}
		}
		if (exitStatus != FAILURE)
		  exitStatus &= fileStatus;
		++optind;
	}
	return exitStatus;	
}

/* Regular expression code calls this routine to print errors. */
void regerror(const char *s) {
	errMsg = (char *) s;
}


