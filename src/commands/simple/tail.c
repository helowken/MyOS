/* tail - copy the end of a file
 *
 *   Syntax:    tail [-f] [-c number | -n number] [file]
 *      tail -[number][c|l|f] [file]       (obsolescent)
 *      tail +[number][c|l|f] [file]       (obsolescent)
 *   Flags:
 *  -c number   Measure starting point in bytes. If number 
 *          begins with '+', the starting point is relative
 *          to the file's beginning. If number begins with 
 *          '-' or has no sign, the starting point is relative
 *          to the end of the file.
 *  -f          Keep trying to read after EOF on files and 
 *          FIFOs.
 *  -n number   Measure starting point in lines. The number
 *          following the flag has significance similar to
 *          that described for the -c flag.
 *   
 *   If neither -c nor -n are specified, the default is tail -n 10.
 */

#ifndef _POSIX_SOURCE
#define _POSIX_SOURCE	1
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef LINE_MAX
#define LINE_MAX	2048
#endif

/* Magic numbers suggested or required by Posix specification */
#define DEFAULT_COUNT	10	/* Default number of lines or bytes */
#define MIN_BUFSIZE		(LINE_MAX * DEFAULT_COUNT)
#define SLEEP_INTERVAL	1	/* Sleep for one second intervals with -f */

#define FALSE	0
#define TRUE	1


static void usage() {
	fputs("Usage: tail [-f] [-c number | -n number] [file]\n", stderr);
	exit(EXIT_FAILURE);
}

static int keepReading() {
/* Wake at intervals to reread standard input. Copy anything read to
 * standard output and then go to sleep again.
 */
	char buf[1024];
	int n;
	int i;
	off_t pos;
	struct stat st;

	pos = lseek(STDIN_FILENO, 0, SEEK_CUR);
	for (;;) {
		for (i = 0; i < 60; ++i) {
			while ((n = read(0, buf, sizeof(buf))) > 0) {
				if (write(STDOUT_FILENO, buf, n) < 0)
				  return EXIT_FAILURE;
			}
			if (n < 0)
			  return EXIT_FAILURE;

			sleep(SLEEP_INTERVAL);
		}
		
		/* Rewind if suddenly truncated. */
		if (pos != -1) {
			if (fstat(STDIN_FILENO, &st) == -1) {
				pos = -1;
			} else if (st.st_size < pos) {
				pos = lseek(STDIN_FILENO, 0, SEEK_SET);
			} else {
				pos = st.st_size;
			}
		}
	}
}

static int tail(int count, int wantBytes, int readUntilKilled) {
	int c;
	char *buf;				/* Pointer to input buffer */
	char *bufEnd;			/* and one past its end */
	char *start;			/* Pointer to first desired character in buf */
	char *finish;			/* Pointer past last desired character */
	int wrappedOnce = FALSE;	/* TRUE after buf has been filled once */

/* This is magic. If count is positive, it means start at the count'th
 * line or byte, with the first line or byte considered number 1. Thus,
 * we want to SKIP one less line or byte than the number specified. In
 * the negative case, we look backward from the end of the file for the
 * (count + 1)'th newline or byte, so we really want the count to be one
 * LARGER than was specified (in absolute value). In either case, the
 * right thing to do is:
 */
	--count;

	/* Count is positive: skip the desired lines or bytes and then copy. */
	if (count >= 0) {
		while (count > 0 && (c = getchar()) != EOF) {
			if (wantBytes || c == '\n')
			  --count;
		}
		while ((c = getchar()) != EOF) {
			if (putchar(c) == EOF)
			  return EXIT_FAILURE;
		}
		if (readUntilKilled)
		  return keepReading();
		return ferror(stdin) ? EXIT_FAILURE : EXIT_SUCCESS;
	}

	/* Count is negative: allocate a reasonably large buffer. */
	if ((buf = (char *) malloc(MIN_BUFSIZE + 1)) == NULL) {
		fputs("tail: out of memory\n", stderr);
		return EXIT_FAILURE;
	}
	bufEnd = buf + MIN_BUFSIZE + 1;

	/* Read the entire file into the buffer. */
	finish = buf;
	while ((c = getchar()) != EOF) {
		*finish++ = c;
		if (finish == bufEnd) {
			finish = buf;
			wrappedOnce = TRUE;
		}
	}
	if (ferror(stdin))
	  return EXIT_FAILURE;

	if (finish != buf || wrappedOnce) {	/* File was not empty */
		start = (finish == buf ? bufEnd - 1 : finish - 1);
		while (start != finish) {
			if ((wantBytes || *start == '\n') && ++count == 0)
			  break;
			if (start == buf) {
				start = bufEnd - 1;
				if (! wrappedOnce)	/* Never wrapped: stop now */
				  break;
			} else {
				--start;
			}
		}
		if (++start == bufEnd) {	/* Bump after going too far */
			start = buf;
		}
		if (finish > start) {
			fwrite(start, 1, finish - start, stdout);
		} else {
			fwrite(start, 1, bufEnd - start, stdout);
			fwrite(buf, 1, finish - buf, stdout);
		}
	}
	if (readUntilKilled)
	  return keepReading();
	return ferror(stdout) ? EXIT_FAILURE : EXIT_SUCCESS;
}

void main(int argc, char **argv) {
	int cFlag = FALSE;
	int nFlag = FALSE;
	int fFlag = FALSE;
	int number = -DEFAULT_COUNT;
	char *suffix;
	int opt;
	struct stat st;

/* Determining whether this invocation is via the standard syntax or
 * via an obsolescent one is a nasty kludge. Here it is, but there is
 * no pretense at elegance.
 */
	if (argc == 1) 	/* Simple: default read of a pipe */
	  exit(tail(-DEFAULT_COUNT, 0, fFlag));

	if (argv[1][0] == '+' || 
			(argv[1][0] == '-' && (
					isdigit(argv[1][1]) || 
					argv[1][1] == 'l' ||
					(argv[1][1] == 'c' && argv[1][2] == 'f')
				)
			)
	) {
		--argc;
		++argv;
		if (isdigit(argv[0][1])) {
			number = (int) strtol(argv[0], &suffix, 10);
			if (number == 0) {		/* Historical */
				if (argv[0][0] == '+')
				  number = 1;
				else
				  exit(EXIT_SUCCESS);
			}
		} else {
			number = (argv[0][0] == '+' ? DEFAULT_COUNT : -DEFAULT_COUNT);
			suffix = &(argv[0][1]);
		}
		if (*suffix != '\0') {
			if (*suffix == 'c') {
				cFlag = TRUE;
				++suffix;
			} else if (*suffix == 'l') {
				nFlag = TRUE;
				++suffix;
			}
		}
		if (*suffix != '\0') {
			if (*suffix == 'f') {
				fFlag = TRUE;
				++suffix;
			}
		}
		if (*suffix != '\0') {	/* Bad form: assume to be a file name */
			number = -DEFAULT_COUNT;
			cFlag = nFlag = fFlag = FALSE;
		} else {
			--argc;
			++argv;
		}
	} else {	/* New Standard syntax */
		while ((opt = getopt(argc, argv, "c:fn:")) != EOF) {
			switch (opt) {
				case 'c':
				case 'n':
					if (opt == 'c') 
					  cFlag = TRUE;
					else
					  nFlag = TRUE;
					if (*optarg == '+' || *optarg == '-')
					  number = atoi(optarg);
					else if (isdigit(*optarg))
					  number = -atoi(optarg);
					else
					  usage();
					if (number == 0) {	/* Historical */
						if (*optarg == '+')
						  number = 1;
						else
						  exit(EXIT_SUCCESS);
					}
					break;
				case 'f':
					fFlag = TRUE;
					break;
				default:
					usage();
			}
		}
		argc -= optind;
		argv += optind;
	}

	if (argc > 1 ||		/* Too many arguments */
		(cFlag && nFlag)) 	/* Both bytes and lines specified */
	  usage();

	if (argc > 0) {		/* An actual file */
		if (freopen(argv[0], "r", stdin) != stdin) {
			fputs("tail: could not open ", stderr);
			fputs(argv[0], stderr);
			fputs("\n", stderr);
			exit(EXIT_FAILURE);
		}
		if (number < 0 && fstat(fileno(stdin), &st) == 0) {
			long offset = cFlag ? (long) number : (long) number * LINE_MAX;

			if (-offset < st.st_size)
			  fseek(stdin, offset, SEEK_END);
		}
	} else {
		fFlag = FALSE;		/* Force -f off when reading a pipe */
	}
	exit(tail(number, cFlag, fFlag));
}






