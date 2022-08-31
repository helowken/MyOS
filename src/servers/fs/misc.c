#include "fs.h"
#include "fcntl.h"
#include "unistd.h"
#include "minix/callnr.h"
#include "minix/com.h"
#include "sys/svrctl.h"
#include "param.h"

int doSync() {
/* Perform the sync() system call. Flush all the tables.
 * The order in which the various tables are flushed is critical. The
 * blocks must be flushed last, since rwInode() leaves its results in
 * the block cache.
 */
	register Inode *ip;
	register Buf *bp;

	/* Write all the dirty inodes to the disk. */
	for (ip = &inodes[0]; ip < &inodes[NR_INODES]; ++ip) {
		if (ip->i_count > 0 && ip->i_dirty == DIRTY)
		  rwInode(ip, WRITING);
	}

	/* Write all the dirty blocks to the disk, one drive at a time. */
	for (bp = &bufs[0]; bp < &bufs[NR_BUFS]; ++bp) {
		if (bp->b_dev != NO_DEV && bp->b_dirty == DIRTY)
		  flushAll(bp->b_dev);
	}

	return OK;	/* sync() can't fail */
}

int doFsync() {
/* Perform the fsync() system call. For now, don't be unnecessarily smart. */
	doSync();
	return OK;
}

int doFcntl() {
/* Perform the fcntl(fd, request, ...) system call. */
	
	register Filp *fp;
	int r, newFd, flags;
	long cloexecMask;	/* Bit map for the FD_CLOEXEC flag */
	long cloexecValue;	/* FD_CLOEXEC flag in proper position */

	/* Is the file descriptor valid? */
	if ((fp = getFilp(inMsg.m_fd)) == NIL_FILP)
	  return errCode;
	
	switch (inMsg.request) {
		case F_DUPFD:
			if (inMsg.addr < 0 || inMsg.addr >= OPEN_MAX)
			  return EINVAL;
			if ((r = getFd(inMsg.addr, 0, &newFd, NULL)) != OK)
			  return r;
			++fp->filp_count;
			currFp->fp_filp[newFd] = fp;
			return newFd;

		case F_GETFD:
		/* Get close-on-exec flag. */
			return ((currFp->fp_cloexec >> inMsg.m_fd) & 01) ? FD_CLOEXEC : 0;

		case F_SETFD:
		/* Set close-on-exec flag. */
			cloexecMask = 1L << inMsg.m_fd;
			cloexecValue = (inMsg.addr & FD_CLOEXEC ? cloexecMask : 0L);
			currFp->fp_cloexec = (currFp->fp_cloexec & ~cloexecMask) | cloexecValue;
			return OK;
		
		case F_GETFL:
		/* Get file status flags (O_NONBLOCK and O_APPEND). */
			flags = fp->filp_flags & (O_NONBLOCK | O_APPEND | O_ACCMODE);
			return flags;

		case F_SETFL:
		/* Set file status flags (O_NONBLOCK and O_APPEND). */
			flags = O_NONBLOCK | O_APPEND;
			fp->filp_flags = (fp->filp_flags & ~flags) | (inMsg.addr & flags);
			return OK;

		case F_GETLK:
		case F_SETLK:
		case F_SETLKW:
			//TODO
		default:
			return EINVAL;
	}
}

int doSet() {
/* Set uid_t or gid_t field. */
	register FProc *rfp;

	/* Only PM may make this call directly. */
	if (who != PM_PROC_NR)
	  return EGENERIC;

	rfp = &fprocTable[inMsg.slot1];
	if (callNum == SETUID) {
		rfp->fp_ruid = (uid_t) inMsg.m_ruid;
		rfp->fp_euid = (uid_t) inMsg.m_euid;
	} else if (callNum == SETGID) {
		rfp->fp_rgid = (gid_t) inMsg.m_rgid;
		rfp->fp_egid = (gid_t) inMsg.m_egid;
	}
	return OK;
}

int doFork() {
/* Perform those aspects of the fork() system call that relate to files.
 * In particular, let the child inherit its parent's file descriptors.
 * The parent and child parameters tell who forked off whom. The file
 * system uses the same slot numbers as the kernel. Only MM makes this call.
 */
	register FProc *cp;
	int i;

	/* Only PM may make this call directly. */
	if (who != PM_PROC_NR)
	  return EGENERIC;

	/* Copy the parent's FProc struct to the child. */
	fprocTable[inMsg.child] = fprocTable[inMsg.parent];

	/* Increase the counters in the 'filp' table. */
	cp = &fprocTable[inMsg.child];
	for (i = 0; i < OPEN_MAX; ++i) {
		if (cp->fp_filp[i] != NIL_FILP)
		  ++cp->fp_filp[i]->filp_count;
	}

	/* Fill in new prcoess id */
	cp->fp_pid = inMsg.pid;

	/* A child is not a process leader. */
	cp->fp_session_leader = 0;

	/* Record the fact that both root and working dir have another user. */
	dupInode(cp->fp_root_dir);
	dupInode(cp->fp_work_dir);

	return OK;
}







