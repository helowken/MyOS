#define _POSIX_C_SOURCE	2
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <utime.h>
#include <minix/minlib.h>

#define val2(string)	((string)[0] * 10 + (string)[1] - '0' * 11)
#define val4(string)	(val2(string) * 100 + val2(string + 2))

typedef enum {
	OLD, NEW
} FormatType;

static char *prog;
static int noCreate = 0;
unsigned int toChange = 0;
#define ATIME	1
#define MTIME	2

static void usageErr() {
	fprintf(stderr, 
		"Usage: %s [-c] [-a] [-m] [-r file] [-t [CC[YY]]MMDDhhmm[.ss]] "
					"MMDDhhmm[YY]] file...\n", prog);
	exit(1);
}

static time_t parseTime(const char *string, FormatType format) {
	struct tm tm;
	time_t touchTime;
	size_t len;

	len = strspn(string, "0123456789");
	if (len % 2 == 1)
	  return -1;
	if (string[len] != '\0' &&
			(string[len] != '.' || format == OLD)) 
	  return -1;

	if (format == OLD) {
		if (len == 10) {
			/* Last two digits are year */
			tm.tm_year = val2(string + 8);
			if (tm.tm_year <= 68)
			  tm.tm_year += 100;
		} else if (len == 8) {
			time(&touchTime);
			tm = *localtime(&touchTime);
		} else 
		  return -1;
	} else {
		if (len == 12) {
			/* First four digits are year */
			tm.tm_year = val4(string) - 1900;
			string += 4;
		} else if (len == 10) {
			/* First two digits are year */
			tm.tm_year = val2(string);
			if (tm.tm_year <= 68)
			  tm.tm_year += 100;
			string += 2;
		} else if (len == 8) {
			time(&touchTime);
			tm = *localtime(&touchTime);
		} else
		  return -1;
	}
	tm.tm_mon = val2(string) - 1;
	string += 2;
	tm.tm_mday = val2(string);
	string += 2;
	tm.tm_hour = val2(string);
	string += 2;
	tm.tm_min = val2(string);
	string += 2;
	if (format == NEW && string[0] == '.') {
		if (isdigit(string[1]) && 
				isdigit(string[2]) &&
				string[3] == '\0') {
			tm.tm_sec = val2(string + 1);
		} else
		  return -1;
	} else {
		tm.tm_sec = 0;
	}
	tm.tm_isdst = -1;

	touchTime = mktime(&tm);
	return touchTime;
}

static int doIt(char *name, struct utimbuf tvp) {
	int fd;
	struct stat sb;

	if (toChange != (ATIME | MTIME)) {
		if (stat(name, &sb) != -1) {
			if (! (toChange & ATIME)) 
			  tvp.actime = sb.st_atime;
			else
			  tvp.modtime = sb.st_mtime;
		}
	}
	if (utime(name, &tvp) == 0)
	  return 0;
	if (errno != ENOENT)
	  return 1;
	if (noCreate == 1)
	  return 0;
	if ((fd = creat(name, 0666)) >= 0) {
		if (fstat(fd, &sb) != -1) {
			if (! (toChange & ATIME)) 
			  tvp.actime = sb.st_atime;
			else
			  tvp.modtime = sb.st_mtime;
		} 
		close(fd);
		if (utime(name, &tvp) == 0)
		  return 0;
	}
	return 1;
}

int main(int argc, char **argv) {
	time_t auxtime;
	struct stat sb;
	int c;
	struct utimbuf touchTimes;
	int fail = 0;

	prog = getProg(argv);
	auxtime = time((time_t *) NULL);
	touchTimes.modtime = auxtime;
	touchTimes.actime = auxtime;

	while ((c = getopt(argc, argv, "r:t:acm0")) != EOF) {
		switch (c) {
			case 'r':
				if (stat(optarg, &sb) == -1) {
					fprintf(stderr, "%s: cannot stat %s: %s\n",
						prog, optarg, strerror(errno));
					exit(1);
				}
				touchTimes.modtime = sb.st_mtime;
				touchTimes.actime = sb.st_atime;
				break;
			case 't':
				auxtime = parseTime(optarg, NEW);
				if (auxtime == (time_t) -1)
				  usageErr();
				touchTimes.modtime = auxtime;
				touchTimes.actime = auxtime;
				break;
			case 'a':
				toChange |= ATIME;
				break;
			case 'm':
				toChange |= MTIME;
				break;
			case 'c':
				noCreate = 1;
				break;
			case '0':
				touchTimes.modtime = touchTimes.actime = 0;
				break;
			case '?':
				usageErr();
				break;
		}
	}
	if (toChange == 0) 
	  toChange = ATIME | MTIME;

	if (optind == argc)
	  usageErr();

	/* Now check for old style time argument */
	if (strcmp(argv[optind - 1], "--") != 0 &&
			(auxtime = parseTime(argv[optind], OLD)) != (time_t) -1) {
		touchTimes.modtime = auxtime;
		touchTimes.actime = auxtime;
		++optind;
		if (optind == argc)
		  usageErr();
	}
	while (optind < argc) {
		/* Copy touchTimes to doIt() since we may change it for each file. */
		if (doIt(argv[optind], touchTimes) > 0) {
			fprintf(stderr, "%s: cannot touch %s: %s\n", 
				prog, argv[optind], strerror(errno));
			fail = 1;
		}
		++optind;
	}
	return fail ? 1 : 0;
}
