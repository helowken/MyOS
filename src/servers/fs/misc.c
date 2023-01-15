#include "fs.h"
#include "fcntl.h"
#include "unistd.h"
#include "minix/callnr.h"
#include "minix/com.h"
#include "sys/svrctl.h"
#include "param.h"

int doExit() {
/* Perform the file system portion of the exit(status) system call. */
	register int i, exitee, task;
	register FProc *rfp;
	register Filp *rfilp;
	register Inode *rip;
	dev_t dev;

	/* Only PM may do the EXIT call directly. */
	if (who != PM_PROC_NR)
	  return EGENERIC;

	/* Nevertheless, pretend that the call came from the user. */
	exitee = inMsg.m_slot1;
	currFp = &fprocTable[exitee];	/* getFilp() needs 'currFp' */

	if (currFp->fp_suspended == SUSPENDED) {
		task = -currFp->fp_task;
		if (task == XPIPE || task == XPOPEN)
		  --suspendedCount;
		inMsg.m_pro = exitee;
		doUnpause();	/* This always succeeds for MM */
		currFp->fp_suspended = NOT_SUSPENDED;
	}

	/* Loop on file descriptors, closing any that are open. */
	for (i = 0; i < OPEN_MAX; ++i) {
		inMsg.m_fd = i;
		doClose();
	}

	/* Release root and working directories. */
	putInode(currFp->fp_root_dir);
	putInode(currFp->fp_work_dir);
	currFp->fp_root_dir = NIL_INODE;
	currFp->fp_work_dir = NIL_INODE;

	/* If a session leader exits then revoke access to its controlling 
	 * tty from all other processes using it.
	 */
	if (! currFp->fp_session_leader) {
		currFp->fp_pid = PID_FREE;
		return OK;		/* Not a session leader. */
	}
	currFp->fp_session_leader = FALSE;
	if (currFp->fp_tty == 0) {
		currFp->fp_pid = PID_FREE;
		return OK;		/* No controlling tty */
	}
	dev = currFp->fp_tty;

	for (rfp = &fprocTable[0]; rfp < &fprocTable[NR_PROCS]; ++rfp) {
		if (rfp->fp_tty == dev)
		  rfp->fp_tty = 0;

		for (i = 0; i < OPEN_MAX; ++i) {
			if ((rfilp = rfp->fp_filp[i]) == NIL_FILP ||
				rfilp->filp_mode == FILP_CLOSED)
			  continue;

			rip = rfilp->filp_inode;
			if ((rip->i_mode & I_TYPE) != I_CHAR_SPECIAL ||
				(dev_t) rip->i_zone[0] != dev)
			  continue;

			devClose(dev);
			rfilp->filp_mode = FILP_CLOSED;
		}
	}

	/* Mark slot as free. */
	currFp->fp_pid = PID_FREE;
	return OK;
}

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
	
	register Filp *filp;
	int r, newFd, flags;
	long cloexecMask;	/* Bit map for the FD_CLOEXEC flag */
	long cloexecValue;	/* FD_CLOEXEC flag in proper position */

	/* Is the file descriptor valid? */
	if ((filp = getFilp(inMsg.m_fd)) == NIL_FILP)
	  return errCode;
	
	switch (inMsg.m_request) {
		case F_DUPFD:
			if (inMsg.m_addr < 0 || inMsg.m_addr >= OPEN_MAX)
			  return EINVAL;
			if ((r = getFd(inMsg.m_addr, 0, &newFd, NULL)) != OK)
			  return r;
			++filp->filp_count;
			currFp->fp_filp[newFd] = filp;
			return newFd;

		case F_GETFD:
		/* Get close-on-exec flag. */
			return ((currFp->fp_cloexec >> inMsg.m_fd) & 01) ? FD_CLOEXEC : 0;

		case F_SETFD:
		/* Set close-on-exec flag. */
			cloexecMask = 1L << inMsg.m_fd;
			cloexecValue = (inMsg.m_addr & FD_CLOEXEC ? cloexecMask : 0L);
			currFp->fp_cloexec = (currFp->fp_cloexec & ~cloexecMask) | cloexecValue;
			return OK;
		
		case F_GETFL:
		/* Get file status flags (O_NONBLOCK and O_APPEND). */
			flags = filp->filp_flags & (O_NONBLOCK | O_APPEND | O_ACCMODE);
			return flags;

		case F_SETFL:
		/* Set file status flags (O_NONBLOCK and O_APPEND). */
			flags = O_NONBLOCK | O_APPEND;
			filp->filp_flags = (filp->filp_flags & ~flags) | (inMsg.m_addr & flags);
			return OK;

		case F_GETLK:
		case F_SETLK:
		case F_SETLKW:
			printf("=== fs TODO doFnctl\n");
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

	rfp = &fprocTable[inMsg.m_slot1];
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
	fprocTable[inMsg.m_child] = fprocTable[inMsg.m_parent];

	/* Increase the counters in the 'filp' table. */
	cp = &fprocTable[inMsg.m_child];
	for (i = 0; i < OPEN_MAX; ++i) {
		if (cp->fp_filp[i] != NIL_FILP) {
		  ++cp->fp_filp[i]->filp_count;
		}
	}

	/* Fill in new prcoess id */
	cp->fp_pid = inMsg.m_pid;

	/* A child is not a process leader. */
	cp->fp_session_leader = 0;

	/* Record the fact that both root and working dir have another user. */
	dupInode(cp->fp_root_dir);
	dupInode(cp->fp_work_dir);

	return OK;
}

int doExec() {
/* Files can be marked with the FD_CLOEXEC bit (in currFp->fp_cloexec). When
 * MM does an EXEC, it calls FS to allow FS to find these files and close them.
 */
	register int i;
	long bitmap;

	/* Only PM may make this call directly. */
	if (who != PM_PROC_NR)
	  return EGENERIC;

	/* The array of FD_CLOEXEC bits is in the fp_cloexec bit map. */
	currFp = &fprocTable[inMsg.m_slot1];
	bitmap = currFp->fp_cloexec;
	if (bitmap == 0)
	  return OK;	/* Normal case, no FD_CLOEXECs */

	/* Check the file descriptors one by one for presence of FD_CLOEXEC. */
	for (i = 0; i < OPEN_MAX; ++i) {
		inMsg.m_fd = i;
		if ((bitmap >> i) & 01)
		  doClose();
	}

	return OK;
}






