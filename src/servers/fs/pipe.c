#include "fs.h"
#include <fcntl.h>
#include <signal.h>
#include <minix/callnr.h>
#include <minix/com.h>
#include <sys/select.h>
#include <sys/time.h>
#include "param.h"
#include "select.h"

int doPipe() {
/* Perform the pipe(fds) system call. */
	register FProc *rfp;
	register Inode *rip;
	int r;
	Filp *fdPtr0, *fdPtr1;
	int fds[2];		/* Reply goes here */

	/* Acquire two file descriptors. */
	rfp = currFp;
	if ((r = getFd(0, R_BIT, &fds[0], &fdPtr0)) != OK)
	  return r;
	rfp->fp_filp[fds[0]] = fdPtr0;
	fdPtr0->filp_count = 1;

	if ((r = getFd(0, W_BIT, &fds[1], &fdPtr1)) != OK) {
		rfp->fp_filp[fds[0]] = NIL_FILP;
		fdPtr0->filp_count = 0;
		return r;
	}
	rfp->fp_filp[fds[1]] = fdPtr1;
	fdPtr1->filp_count = 1;

	/* Make the inode on the pipe device. */
	if ((rip = allocInode(rootDev, I_REGULAR)) == NIL_INODE) {
		rfp->fp_filp[fds[0]] = NIL_FILP;
		fdPtr0->filp_count = 0;
		rfp->fp_filp[fds[1]] = NIL_FILP;
		fdPtr1->filp_count = 0;
		return errCode;
	}

	if (checkReadOnly(rip) != OK)
	  panic(__FILE__, "pipe device is read only", NO_NUM);

	rip->i_pipe = I_PIPE;
	rip->i_mode &= ~I_REGULAR;
	rip->i_mode |= I_NAMED_PIPE;	/* Pipes and FIFOs have this bit set */
	fdPtr0->filp_inode = rip;
	fdPtr0->filp_flags = O_RDONLY;
	dupInode(rip);					/* For double usage */
	fdPtr1->filp_inode = rip;
	fdPtr1->filp_flags = O_WRONLY;
	rwInode(rip, WRITING);			/* Mark inode as allocated */

	outMsg.m_reply_i1 = fds[0];
	outMsg.m_reply_i2 = fds[1];
	rip->i_update = ATIME | CTIME | MTIME;
	return OK;
}

int doUnpause() {
/* A signal has been sent to a user who is paused on the file system.
 * Abort the system call with the EINTR error message.
 */
	register FProc *rfp;
	int pNum, task, fd;
	Filp *filp;
	dev_t dev;
	Message msg;

	if (who > PM_PROC_NR)
	  return EPERM;
	pNum = inMsg.m_pro;
	if (pNum < 0 || pNum >= NR_PROCS)
	  panic(__FILE__, "unpause err 1", pNum);
	rfp = &fprocTable[pNum];
	if (rfp->fp_suspended == NOT_SUSPENDED)
	  return OK;
	task = -rfp->fp_task;

	switch (task) {
		case XPIPE:		/* Process trying to read or write a pipe */
			break;
		case XLOCK:		/* Process trying to set a lock with FCNTL */
			break;
		case XSELECT:	/* Process blocking on select() */
			selectForget(pNum);
			break;
		case XPOPEN:	/* Process trying to open a fifo */
			break;
		default:		/* Process trying to do device I/O (e.g. tty) */
			fd = FD_FROM_SUSP(rfp->fp_fd);	/* Extract fd */
			if (fd < 0 || fd >= OPEN_MAX)
			  panic(__FILE__, "unpause err 2", NO_NUM);
			filp = rfp->fp_filp[fd];
			dev = (dev_t) filp->filp_inode->i_zone[0];	/* Device hung on */
			msg.TTY_LINE = MINOR_DEV(dev);
			msg.PROC_NR = pNum;

			/* Tell kernel R or W. Mode is from current call, not open. */
			msg.COUNT = CALL_FROM_SUSP(rfp->fp_fd) == READ ? R_BIT : W_BIT;
			msg.m_type = CANCEL;
			currFp = rfp;	/* Hack - ctty_io uses currFp */
			(*dmapTable[MAJOR_DEV(dev)].dmap_io)(task, &msg);
	}

	rfp->fp_suspended = NOT_SUSPENDED;
	reply(pNum, EINTR);		/* Signal interrupted call */
	return OK;
}

void suspend(
	int task	/* Who is proc waiting for? (PIPE = pipe) */
) {
/* Take measures to suspend the processing of the present system call.
 * Store the parameters to be used upon resuming in the process table.
 * (Actually they are not used when a process is waiting for an I/O device,
 * but they are needed for pipes, and it is not worth making the distinction.)
 * The SUSPEND pseudo error should be returned after calling suspend().
 */
	if (task == XPIPE || task == XPOPEN)
	  ++suspendedCount;		/* # procs suspended on pipe */
	currFp->fp_suspended = SUSPENDED;
	currFp->fp_fd = TO_SUSP(inMsg.m_fd, callNum);
	currFp->fp_task = -task;
	if (task == XLOCK) {
		currFp->fp_buffer = (char *) inMsg.m_name1;	/* Third arg to fcntl() */
		currFp->fp_nbytes = inMsg.m_request;	/* Second arg to fcntl() */
	} else {
		currFp->fp_buffer = inMsg.m_buffer;		/* For reads and writes */
		currFp->fp_nbytes = inMsg.m_nbytes;
	}
}

void revive(
	int pNum,	/* Process to revive */
	int retVal	/* If hanging on task, how many bytes read */
) {
/* Revive a previously blocked process. When a process hangs on tty, this
 * is the way it is eventually released.
 */
	register FProc *rfp;
	register int task;

	if (pNum < 0 || pNum >= NR_PROCS)
	  panic(__FILE__, "revive err", pNum);

	rfp = &fprocTable[pNum];
	if (rfp->fp_suspended == NOT_SUSPENDED || rfp->fp_revived == REVIVING)
	  return;

	/* The 'reviving' flag only applies to pipes. Processes waiting for TTY get
	 * a message right away. The revival process is different for TTY and pipes.
	 * For select and TTY revival, the work is already done, for pipes it is not:
	 *  the proc must be restarted so it can try again.
	 */
	task = -rfp->fp_task;
	if (task == XPIPE || task == XLOCK) {
		/* Revive a process suspended on a pipe or lock. */
		rfp->fp_revived = REVIVING;
		++reviving;		/* Process was waiting on pipe or lock. */
	} else {
		rfp->fp_suspended = NOT_SUSPENDED;
		if (task == XPOPEN)	/* Process blocked in open or create */
		  reply(pNum, FD_FROM_SUSP(rfp->fp_fd));
		else if (task == XSELECT)
		  reply(pNum, retVal);
		else {
			/* Revive a process suspended on TTY or other device. */
			rfp->fp_nbytes = retVal;	/* Pretend it wants only what there is */
			reply(pNum, retVal);
		}
	}
}

int pipeCheck(
	register Inode *ip,	/* The inode of the pipe */
	int rwFlag,			/* READING or WRITING */
	int oFlags,			/* Flags set by open or fcntl */
	int bytes,			/* Bytes to be read or written (all chunks) */
	off_t position,		/* Current file position */
	int *bytesCanWrite,	/* Return: number of bytes we can write */
	int noTouch			/* Check only */
) {
/* Pipes are a little different. If a process reads from an empty pipe for
 * which a writer still exists, suspend the reader. If the pipe is empty
 * and there is no writer, return 0 bytes. If a process is writing to a
 * pipe and no one is reading from it, give a broken pipe error.
 */
	/* If reading, check for empty pipe. */
	if (rwFlag == READING) {
		if (position >= ip->i_size) {
			/* Process is reading from an empty pipe. */
			int r = 0;
			if (findFilp(ip, W_BIT) != NIL_FILP) {
				/* Writer exists */
				if (oFlags & O_NONBLOCK) {
					r = EAGAIN;
				} else {
					if (! noTouch)
					  suspend(XPIPE);	/* Block reader */
					r = SUSPEND;
				}
				/* If need be, activate sleeping writers. */
				if (suspendedCount > 0 && ! noTouch) 
				  release(ip, WRITE, suspendedCount);
			}
			return r;
		}
	} else {
		/* Process is writing to a pipe. */
		if (findFilp(ip, R_BIT) == NIL_FILP) {
			/* Tell kernel to generate a SIGPIPE signal. */
			if (! noTouch)
			  sysKill((int) (currFp - fprocTable), SIGPIPE);
			return EPIPE;
		}

		size_t pipeSize = PIPE_SIZE(ip->i_sp->s_block_size);
		if (position + bytes > pipeSize) {
			if ((oFlags & O_NONBLOCK) && bytes < pipeSize) { 
				return EAGAIN;
			} else if ((oFlags & O_NONBLOCK) && bytes > pipeSize) {
				if ((*bytesCanWrite = pipeSize - position) > 0) {
					/* Do a partial write. Need to wakeup reader. */
					if (! noTouch)
					  release(ip, READ, suspendedCount);
					return 1;
				} else {
					return EAGAIN;
				}
			}
			if (bytes > pipeSize) {
				if ((*bytesCanWrite = pipeSize - position) > 0) {
					/* Do a partial write. Need to wakeup reader
					 * since we'll suspend ourself in readWrite().
					 */
					release(ip, READ, suspendedCount);
					return 1;
				}
			}
			if (! noTouch) 
			  suspend(XPIPE);	/* Stop writer -- pipe full */
			return SUSPEND;
		}

		/* Writing to an empty pipe. Search for suspended reader. */
		if (position == 0 && ! noTouch) 
		  release(ip, READ, suspendedCount);
	}

	*bytesCanWrite = 0;
	return 1;
}

void release(
	register Inode *ip,	/* Inode of pipe */
	int callNum,		/* READ, WRITE, OPEN or CREAT */
	int count			/* Max number of processes to release */
) {
/* Check to see if any process is hanging on the pipe whose inode is in 'ip'.
 * If one is, and it was trying to perform the call indicated by 'callNum',
 * release it.
 */
	register FProc *rp;
	Filp *filp;

	/* Trying to perform the call also includes SELECTing on it with that
	 * operation.
	 */
	if (callNum == READ || callNum == WRITE) {
		int op;
		if (callNum == READ)
		  op = SEL_RD;
		else 
		  op = SEL_WR;

		for (filp = &filpTable[0]; filp < &filpTable[NR_FILPS]; ++filp) {
			if (filp->filp_count < 1 ||
					! (filp->filp_pipe_select_ops & op) ||
					filp->filp_inode != ip)
			  continue;

			selectCallback(filp, op);
			filp->filp_pipe_select_ops &= ~op;
		}
	}

	/* Search the proc table. */
	for (rp = &fprocTable[0]; rp < &fprocTable[NR_PROCS]; ++rp) {
		if (rp->fp_suspended == SUSPENDED &&
				rp->fp_revived == NOT_REVIVING &&
				(rp->fp_fd & BYTE) == callNum &&
				rp->fp_filp[FD_FROM_SUSP(rp->fp_fd)]->filp_inode == ip) {
			revive((int) (rp - fprocTable), 0);
			--suspendedCount;	/* Keep track of who is suspended */
			if (--count == 0)
			  return;
		}
	}
}

