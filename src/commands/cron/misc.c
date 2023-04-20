#define nil	((void *) 0)
#include "sys/types.h"
#include "stdio.h"
#include "stdarg.h"
#include "stdlib.h"
#include "time.h"
#include "misc.h"

char *progName;		/* Name of this program. */
time_t now;			/* Cron's idea of the current time. */
time_t nextTime;	/* Time to run the next job. */

size_t allocCount;	/* # of chunks of memory allocated. */

void *allocate(size_t len) {
	void *mem;

	if ((mem = malloc(len)) == nil) {
		log(LOG_ALERT, "Out of memory, exiting\n");
		exit(EXIT_FAILURE);
	}
	++allocCount;
	return mem;
}

void deallocate(void *mem) {
	if (mem != nil) {
		free(mem);
		--allocCount;
	}
}

static LogTo logTo = SYSLOG;

void selectLog(LogTo where) {
/* Select where logging output should go, syslog or stdout. */
	logTo = where;
}

void log(int level, const char *fmt, ...) {
/* Like syslog(), but may go to stderr. */
	va_list ap;

	va_start(ap, fmt);
	fprintf(stderr, "%s: ", progName);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}

