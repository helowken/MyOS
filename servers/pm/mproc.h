#include "timers.h"

typedef struct MProc MProc;

EXTERN struct MProc {
	MemMap mp_seg[NR_LOCAL_SEGS];	/* Points to text, data, stack */
	pid_t mp_pid;			/* Process id */
	pid_t mp_proc_grp;		/* Pid of process group (used for signals) */
	pid_t mp_wait_pid;		/* Pid this process is waiting for */
	int mp_parent;			/* Index of parent process */

	/* Child user and system times. Accounting done on child exit. */
	clock_t mp_child_utime;		/* Cumulative user time of children */
	clock_t mp_child_stime;		/* Cumulative system time of children */

	/* Real and effective uids and gids. */
	uid_t mp_ruid;			/* Process' real uid */
	uid_t mp_euid;			/* Process' effective uid */
	gid_t mp_rgid;			/* Process' real gid */
	gid_t mp_egid;			/* Process' effective gid */

	/* Signal handling information. */
	Timer mp_timer;			/* Watchdog timer for alarm */

	/* Scheduling priority. */
	int mp_nice;			/* Nice is PRIO_MIN..PRIO_MAX, standard 0. */

	char mp_name[PROC_NAME_LEN];	/* Process name */
} mprocTable[NR_PROCS];

