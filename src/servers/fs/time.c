#include "fs.h"
#include "fcntl.h"
#include "minix/callnr.h"
#include "minix/com.h"
#include "param.h"

int doSTime() {
/* Perform the stime(tp) system call. */
	bootTime = (long) inMsg.m_pm_stime;
	return OK;
}

int doUTime() {
/* Perform the utime(name, timep) system call. */
	register Inode *rip;
	register int len, r;

	/* Adjust for case of 'timep' being NULL;
	 * m_utime_strlen then holds the actual size: strlen(name) + 1.
	 */
	len = inMsg.m_utime_length;
	if (len == 0)
	  len = inMsg.m_utime_strlen;

	/* Temporarily open the file. */
	if (fetchName(inMsg.m_utime_file, len, M1) != OK)
	  return errCode;
	if ((rip = eatPath(userPath)) == NIL_INODE)
	  return errCode;

	/* Only the owner of a file or the superUser can change its time. */
	r = OK;
	if (rip->i_uid != currFp->fp_euid && ! superUser)
	  r = EPERM;
	if (inMsg.m_utime_length == 0 && r != OK)
	  r = checkForbidden(rip, W_BIT);
	if (checkReadOnly(rip) != OK)
	  r = EROFS;	/* Not even su can touch if R/O */
	if (r == OK) {
		if (inMsg.m_utime_length == 0) {
			rip->i_atime = clockTime();
			rip->i_mtime = rip->i_atime;
		} else {
			rip->i_atime = inMsg.m_utime_actime;
			rip->i_mtime = inMsg.m_utime_modtime;
		}
		rip->i_update |= CTIME;		/* Discard any stale ATIME and MTIME flags */
		rip->i_dirty = DIRTY;
	}
	putInode(rip);
	return r;
}
