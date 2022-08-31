#include "pm.h"
#include "sys/stat.h"
#include "minix/callnr.h"
#include "minix/com.h"
#include "signal.h"
#include "string.h"
#include "mproc.h"
#include "param.h"

MProc *findShare(MProc *rmp, ino_t ino, dev_t dev, time_t ctime) {
/* Look for a process that is the file <ino, dev, ctime> in execute. Don't
 * accidentally "find" rmp, because it is the process on whose behalf this
 * call is made.
 */
	MProc *shareMp;
	for (shareMp = &mprocTable[0]; shareMp < &mprocTable[NR_PROCS]; ++ shareMp) {
		if (shareMp == rmp ||
			shareMp->mp_ino != ino ||
			shareMp->mp_dev != dev ||
			shareMp->mp_ctime != ctime)
		  continue;
		return shareMp;
	}
	return NULL;
}

int doExec() {
/* Perform the execve(name, argv, envp) call. The user library builds a
 * complete stack image, including pointers, args, environ, etc. The stack
 * is copied to a buffer inside PM, and then to the new core image.
 */
	//register MProc *rmp;
	static char mBuf[ARG_MAX];		/* Buffer for stack and zeroes */
	static char nameBuf[PATH_MAX];	/* The name of the file to exec */
	vir_bytes src, dst, stackBytes;
	char *name;
	int r, fd;
	struct stat stBuf[2], *stp;

	/* Do some validity checks. */
	//rmp = currMp;
	stackBytes = (vir_bytes) inMsg.stack_bytes;
	if (stackBytes > ARG_MAX)
	  return ENOMEM;	/* Stack too big */
	if (inMsg.exec_len <= 0 || inMsg.exec_len > PATH_MAX)
	  return EINVAL;

	/* Get the exec file name and see if the file is executable. */
	src = (vir_bytes) inMsg.exec_name;
	dst = (vir_bytes) nameBuf;
	if ((r = sysDataCopy(who, src, PM_PROC_NR, dst,
				(phys_bytes) inMsg.exec_len)) != OK)
	  return r;		/* File name not in user data segment */
	
	/* Fetch the stack from the user before destroying the old core image. */
	src = (vir_bytes) inMsg.stack_ptr;
	dst = (vir_bytes) mBuf;
	if ((r = sysDataCopy(who, src, PM_PROC_NR, dst,
				(phys_bytes) stackBytes)) != OK)
	  return r;		/* Can't fetch stack (e.g. bad virtual addr) */

	r = 0;	/* r = 0 (frist attemp), or 1 (interpreted script) */
	name = nameBuf;
	do {
		stp = &stBuf[r];
		tellFS(CHDIR, who, false, 0);	/* Switch to the user's FS environ */
		fd = checkAllowed(name, stp, X_BIT);	/* Is file executable? */
		if (fd < 0)
		  return fd;	/* File was not executable */

		printf("=========== fd: %d\n", fd);
	} while (false);//TODO


	return EACCES; //TODO
}
