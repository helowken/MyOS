#ifndef MISC__H
#define MISC__H

#include "time.h"

/* The name of the program. */
extern char *progName;

/* Where cron stores its pid. */
#define PID_FILE	"/usr/run/cron.pid"

/* Cron's idea of the current time, and the time next to run something. */
extern time_t now;
extern time_t nextTime;

/* Memory allocation. */
void *allocate(size_t len);
void deallocate(void *mem);
extern size_t allocCount;

/* Logging, by syslog or to stderr. */
typedef enum { LOG_ERR, LOG_CRIT, LOG_ALERT } LogDummy;
typedef enum { SYSLOG, STDERR } LogTo;
void selectLog(LogTo where);
void log(int level, const char *fmt, ...);

#endif
