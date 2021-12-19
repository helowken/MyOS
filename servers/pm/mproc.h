#include "timers.h"
#include "signal.h"

typedef struct MProc MProc;

EXTERN struct MProc {
	MemMap mp_memmap[NR_LOCAL_SEGS];	/* Points to text, data, stack */
	char mp_exit_status;	/* Storage for status when process exits */
	char mp_sig_status;		/* Storage for status # for killed procs */
	pid_t mp_pid;			/* Process id */
	pid_t mp_proc_grp;		/* Pid of process group (used for signals) */
	pid_t mp_wait_pid;		/* Pid this process is waiting for */
	int mp_parent_idx;		/* Index of parent process */

	/* Child user and system times. Accounting done on child exit. */
	clock_t mp_child_utime;		/* Cumulative user time of children */
	clock_t mp_child_stime;		/* Cumulative system time of children */

	/* Real and effective uids and gids. */
	uid_t mp_ruid;			/* Process' real uid */
	uid_t mp_euid;			/* Process' effective uid */
	gid_t mp_rgid;			/* Process' real gid */
	gid_t mp_egid;			/* Process' effective gid */

	/* Signal handling information. */
	sigset_t mp_sig_ignore;		/* 1 means ignore the signal, 0 means don't */
	sigset_t mp_sig_catch;		/* 1 means catch the signal, 0 means don't */
	sigset_t mp_sig_to_msg;		/* 1 means transform into notify message */
	sigset_t mp_sig_mask;		/* Signals to be blocked */
	sigset_t mp_sig_mask2;		/* Saved copy of mp_sig_mask */
	sigset_t mp_sig_pending;	/* Pending signals to be handled */
	struct sigaction mp_sig_actions[NSIG + 1];	/* As in sigaction(2) */
	vir_bytes mp_sig_return;	/* Address of C library _sigreturn function */
	Timer mp_timer;				/* Watchdog timer for alarm */

	unsigned mp_flags;		/* Flag bits */
	MProc *mp_swap_in_q;	/* Queue of procs waiting to be swapped in */
	Message mp_reply;		/* Reply message to be sent to one */

	/* Scheduling priority. */
	int mp_nice;			/* Nice is PRIO_MIN..PRIO_MAX, standard 0. */

	char mp_name[PROC_NAME_LEN];	/* Process name */
} mprocTable[NR_PROCS];

/* Flag values */
#define IN_USE			0x001	/* Set when 'mprocTable' slot in use */
#define WAITING			0x002	/* Set by WAIT system call */
#define ZOMBIE			0x004	/* Set by EXIT, cleared by WAIT */
#define PAUSED			0x008	/* Set by PAUSE system call */
#define ALARM_ON		0x010	/* Set when SIGALRM timer started */
#define TRACED			0x040	/* Set if process is to be traced */
#define STOPPED			0x080	/* Set if process stopped for tracing */
#define SIGSUSPENDED	0x100	/* Set by SIGSUSPEND system call */
#define REPLY			0x200	/* Set if a reply message is pending */
#define ONSWAP			0x400	/* Set if data segment is swapped out */
#define SWAPIN			0x800	/* Set if on the "swap this in" queue */
#define DONT_SWAP		0x1000	/* Never swap out this process */
#define	PRIV_PROC		0x2000	/* System process, special privileges */
