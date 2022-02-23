/* This is the per-process information. A slot is reserved for each potential
 * process. Thus NR_PROCS must be the same as in the kernel. It is not
 * possible or even necessary to tell when a slot is free here.
 */
typedef struct FProc {
	mode_t fp_umask;	/* Mask set by umask system call */
	uid_t fp_ruid;			/* Real user id */
	uid_t fp_euid;			/* Effective user id */
	gid_t fp_rgid;			/* Real group id */
	gid_t fp_egid;			/* Effective group id */
	pid_t fp_pid;		/* Process id */
} FProc;

EXTERN FProc fprocTable[NR_PROCS];

#define isOkProcNum(n)		((unsigned) ((n) + NR_TASKS) < NR_PROCS + NR_TASKS)
