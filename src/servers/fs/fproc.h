/* This is the per-process information. A slot is reserved for each potential
 * process. Thus NR_PROCS must be the same as in the kernel. It is not
 * possible or even necessary to tell when a slot is free here.
 */
typedef struct FProc {
	mode_t fp_umask;		/* Mask set by umask system call */
	Inode *fp_work_dir;		/* Pointer to working directory's inode */
	Inode *fp_root_dir;		/* Pointer to current root dir (see chroot) */
	Filp *fp_filp[OPEN_MAX];	/* The file descriptor table */
	uid_t fp_ruid;			/* Real user id */
	uid_t fp_euid;			/* Effective user id */
	gid_t fp_rgid;			/* Real group id */
	gid_t fp_egid;			/* Effective group id */
	dev_t fp_tty;			/* Major/minor of controlling tty */
	int fp_fd;				/* Place to save fd if rd/wr can't finish */
	char *fp_buffer;		/* Place to save buffer if rd/wr can't finish */
	int fp_nbytes;			/* Place to save bytes if rd/wr can't finish */
	char fp_suspended;		/* Set to indicate process hanging */
	char fp_revived;		/* Set to indicate process being revived */
	char fp_session_leader;	/* true if proc is a session leader */
	pid_t fp_pid;			/* Process id */
	long fp_cloexec;		/* Bit map for POSIX Table 6-2 FD_CLOEXEC */
} FProc;

/* Field values. */
#define NOT_SUSPENDED	0	/* Process is not suspended on pipe or task */
#define SUSPENDED		1	/* Process is suspended on pipe or task */
#define NOT_REVIVING	0	/* Process is not being revived */
#define REVIVING		1   /* Process is being revived from suspension */
#define PID_FREE		0	/* Process slot free */

/* Check is process number is acceptable - includes system processes. */
#define isOkProcNum(n)		((unsigned) ((n) + NR_TASKS) < NR_PROCS + NR_TASKS)
