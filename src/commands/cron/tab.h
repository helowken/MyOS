#ifndef TAB__H
#define TAB__H

struct Crontab;

typedef unsigned char	bitmap_t[8];

typedef struct CronJob {	/* One entry in a crontab file */
	struct CronJob *next;
	struct Crontab *tab;	/* Associated table file. */
	bitmap_t min;			/* Minute (0-59) */
	bitmap_t hour;			/* Hour (0-23) */
	bitmap_t mday;			/* Day of the month (1-31) */
	bitmap_t mon;			/* Month (1-12) */
	bitmap_t wday;			/* Weekday (0-7 iwth 0 = 7 = Sunday) */
	char *user;				/* User to run it as (nil = root) */
	char *cmd;				/* Command to run */
	time_t runTime;			/* When next to run */
	char doMday;			/* True iff mon or mday is not '*' */
	char doWday;			/* True iff wday is not '*' */
	char late;				/* True iff the job is late */
	char atJob;				/* True iff it is an AT job */
	pid_t pid;				/* Process-id of job if nonzero */
} CronJob;

typedef struct Crontab {
	struct Crontab *next;
	char *file;		/* Crontab name */
	char *user;		/* Owner if non-null */
	time_t mtime;	/* Last modified time */
	CronJob	*jobs;	/* List of jobs in the file */
	char *data;		/* File data */
	int current;	/* True if current, i.e. file exists */
} Crontab;

Crontab *crontabs;	/* All crontabs. */

/* A time as far in the future as possible. */
#define NEVER		((time_t) ((time_t) -1 < 0 ? LONG_MAX : ULONG_MAX))

/* Don't trust crontabs bigger than this: */
#define TAB_MAX		1024

/* Pid if no process running, or a pid value you'll never see. */
#define IDLE_PID	((pid_t) 0)
#define NO_PID		((pid_t) -1)

/* bitmap_t operations. */
#define bitSet(map, n)		((void) ((map)[(n) >> 3] |= (1 << ((n) & 7))))
#define bitClear(map, n)	((void) ((map)[(n) >> 3] &= ~(1 << ((n) & 7))))
#define bitIsSet(map, n)	(!!((map)[(n) >> 3] & (1 << ((n) & 7))))

/* Functions. */
void tabParse(char *file, char *user);
void tabFindATJob(char *atDir);
void tabPurge(void);
void tabReapJob(pid_t pid);
void tabReschedule(CronJob *job);
CronJob *tabNextJob(void);
void tabPrint(FILE *fp);

#endif
