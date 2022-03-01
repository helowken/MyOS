/* This is the per-process information. A slot is reserved for each potential
 * process. Thus NR_PROCS must be the same as in the kernel. It is not
 * possible or even necessary to tell when a slot is free here.
 */
typedef struct FProc {
	mode_t fp_umask;	/* Mask set by umask system call */
	Inode *fp_work_dir;	/* Pointer to working directory's inode */
	Inode *fp_root_dir;	/* Pointer to current root dir (see chroot) */
	uid_t fp_ruid;		/* Real user id */
	uid_t fp_euid;		/* Effective user id */
	gid_t fp_rgid;		/* Real group id */
	gid_t fp_egid;		/* Effective group id */
	pid_t fp_pid;		/* Process id */
} FProc;

/* Field values. */
#define NOT_SUSPENDED	0	/* Process is not suspended on pipe or task */
#define SUSPENDED		1	/* Process is suspended on pipe or task */
#define NOT_REVIVING	0	/* Process is not being revived */
#define REVIVING		1   /* Process is being revived from suspension */
#define PID_FREE		0	/* Process slot free */

/* Check is process number is acceptable - includes system processes. */
#define isOkProcNum(n)		((unsigned) ((n) + NR_TASKS) < NR_PROCS + NR_TASKS)
