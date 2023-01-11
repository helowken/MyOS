

#include "sys/types.h"

/* Mode argument to forkShell. Don't change FORK_FG or FORK_BG. */
#define FORK_FG 0
#define FORK_BG 1
#define FORK_NO_JOB 2

/* A job structure contains information about a job. A job is either a
 * single process or a set of processes contained in a pipeline. In the
 * later case, pidList will be non-NULL, and will point to a -1 terminated
 * array of pids.
 */
typedef struct {
	pid_t pid;		/* Process id */
	short status;	/* Status flags (defined above) */
	char *cmd;		/* Text of command being run */
} ProcStat;

/* States */
#define JOB_STOPPED	1	/* All procs are stopped */
#define JOB_DONE	2	/* All procs are completed */

typedef struct Job {	
	ProcStat ps0;		/* Status of process */
	ProcStat *ps;		/* Status or processes when more than one */
	pid_t numProcs;		/* Number of processes */
	pid_t pgrp;			/* Process group this job */
	char state;			/* True if job is finished */
	char used;			/* True if this entry is in used */
	char changed;		/* True if status has changed */
} Job;

extern pid_t backgndPid;		/* Pid of last background process */

void freeJob(Job *);
Job *makeJob(union Node *, int);
int forkShell(Job *, Node *, int);
int waitForJob(Job *);
