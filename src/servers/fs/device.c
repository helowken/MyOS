#include "fs.h"
#include "fcntl.h"
#include "minix/callnr.h"
#include "minix/com.h"
#include "param.h"

int noDev(int op, dev_t dev, int proc, int flags) {
	return ENODEV;
}

int cloneOpCl(int op, dev_t dev, int proc, int flags) {
	printf("=== fs TODO cloneOpCl\n");
//TODO
	return 0;
}

int genOpCl(int op, dev_t dev, int proc, int flags) {
	DMap *dp;
	Message msg;

	/* Determine task dmap. */
	dp = &dmapTable[majorDev(dev)];

	msg.m_type = op;
	msg.DEVICE = minorDev(dev);
	msg.PROC_NR = proc;
	msg.COUNT = flags;

	/* Call the task. */
	(*dp->dmap_io)(dp->dmap_driver, &msg);

	return msg.REP_STATUS;
}

int ttyOpCl(int op, dev_t dev, int proc, int flags) {
/* This procedure is called from the dmap struct on tty open/close. */
	int r;
	register struct FProc *rfp;

	/* Add O_NOCTTY to the flags if this process is not a session leader, or
	 * if it already has a controlling tty, or if it is someone elses
	 * controlling tty.
	 */
	if (!currFp->fp_session_leader || currFp->fp_tty != 0) {
		flags |= O_NOCTTY;
	} else {
		for (rfp = &fprocTable[0]; rfp < &fprocTable[NR_PROCS]; ++rfp) {
			if (rfp->fp_tty == dev)
			  flags |= O_NOCTTY;
		}
	}

	r = genOpCl(op, dev, proc, flags);
	
	/* Did this call make the tty the controlling tty? */
	if (r == 1) {
		currFp->fp_tty = dev;
		r = OK;
	}
	return r;
}

int cttyOpCl(int op, dev_t dev, int proc, int flags) {
	return currFp->fp_tty == 0 ? ENXIO : OK;
}

void genIO(int taskNum, Message *msg) {
/* All file system I/O ultimately comes down to I/O on major/minor device
 * pairs. These lead to calls on the following routines via the dmap table.
 */
	int r, pNum;
	Message recMsg;

	pNum = msg->PROC_NR;
	if (! isOkProcNum(pNum)) {
		printf("FS: warning, got illegal process number (%d) from %d\n",
					pNum, msg->m_source);
		return;
	}

	while ((r = sendRec(taskNum, msg)) == ELOCKED) {
		/* sendRec() failed to avoid deadlock. The task 'taskNum' is
		 * trying to send a REVIVE message for an earlier request.
		 * Handle it and go try again.
		 */
		if ((r = receive(taskNum, &recMsg)) != OK)
		  break;

		/* If we're trying to send a cancel message to a task which has just
		 * sent a completion reply, ignore the reply and abort the cancel
		 * request. The caller will do the revive for the process.
		 */
		if (msg->m_type == CANCEL && recMsg.REP_PROC_NR == pNum) 
		  return;

		/* Otherwise it should be a REVIVE. */
		if (recMsg.m_type != REVIVE) {
			printf("FS: strange device reply from %d, type = %d, proc = %d (1)\n",
					recMsg.m_source, recMsg.m_type, recMsg.REP_PROC_NR);
			continue;
		}

		revive(recMsg.REP_PROC_NR, recMsg.REP_STATUS);
	}

	/* The message received may be a reply to this call, or a REVIVE for some
	 * other process.
	 */
	while (1) {
		if (r != OK) {
			if (r == EDEADDST)
			  return;
			else
			  panic(__FILE__, "callTask: can't send/receive", r);
		}

		/* Did the process we did the sendRec() for get a result? */
		if (msg->REP_PROC_NR == pNum) {
			break;
		} else if (msg->m_type == REVIVE) {
			/* Otherwise it should be a REVIVE. */
			revive(msg->REP_PROC_NR, msg->REP_STATUS);
		} else {
			printf("FS: strange device reply from %d, type = %d, proc = %d (1)\n",
					msg->m_source, msg->m_type, msg->REP_PROC_NR);
			return;
		}

		r = receive(taskNum, msg);
	}
}

void cttyIO(int taskNum, Message *msg) {
/* This routine is only called for one device, namely /dev/tty. Its job
 * is to change the message to use the controlling terminal, instead of the
 * major/minor pair for /dev/tty itself.
 */
	DMap *dp;

	if (currFp->fp_tty == 0) {
		/* No controlling tty present anymore, return an I/O error. */
		msg->REP_STATUS = EIO;
	} else {
		/* Substitute the controlling terminal device. */
		dp = &dmapTable[majorDev(currFp->fp_tty)];
		msg->DEVICE = minorDev(currFp->fp_tty);
		(*dp->dmap_io)(dp->dmap_driver, msg);
	}
}


int devOpen(dev_t dev, int proc, int flags) {
/* Determine the major device number call the device class specific
 * open/close routine. (This is the only routine that must check the
 * device number for being in range. All others can trust this check.)
 */
	int major, r;
	DMap *dp;

	major = majorDev(dev);
	if (major >= NR_DEVICES)
	  major = 0;
	dp = &dmapTable[major];
	if ((r = (*dp->dmap_opcl)(DEV_OPEN, dev, proc, flags)) == SUSPEND)
	  panic(__FILE__, "suspend on open from", dp->dmap_driver);
	return r;
}

void devClose(dev_t dev) {
	(*dmapTable[majorDev(dev)].dmap_opcl)(DEV_CLOSE, dev, 0, 0);
}

int devIO(int op, dev_t dev, int proc, void *buf, 
			off_t pos, int bytes, int flags) {
/* Read or write from a device. The parameter 'dev' tells which one. */
	DMap *dp;
	Message msg;

	/* Determine task dmap. */
	dp = &dmapTable[majorDev(dev)];

	/* Set up the message passed to task. */
	msg.m_type = op;
	msg.DEVICE = minorDev(dev);
	msg.POSITION = pos;
	msg.PROC_NR = proc;
	msg.ADDRESS = buf;
	msg.COUNT = bytes;
	msg.TTY_FLAGS = flags;

	/* Call the task. */
	(*dp->dmap_io)(dp->dmap_driver, &msg);

	/* Task has completed. See if call completed. */
	if (msg.REP_STATUS == SUSPEND) {
		if (flags & O_NONBLOCK) {
			/* Not supposed to block. */
			msg.m_type = CANCEL;
			msg.PROC_NR = proc;
			msg.DEVICE = minorDev(dev);
			(*dp->dmap_io)(dp->dmap_driver, &msg);
			if (msg.REP_STATUS == EINTR)
			  msg.REP_STATUS = EAGAIN;
		} else {
			/* Suspend user. */
			suspend(dp->dmap_driver);
			return SUSPEND;
		}
	}
	return msg.REP_STATUS;
}

void devStatus(Message *msg) {
	Message statusMsg;
	int i, r;
	bool getMore = true;

	for (i = 0; i < NR_DEVICES; ++i) {
		if (dmapTable[i].dmap_driver == msg->m_source)
		  break;
	}
	if (i >= NR_DEVICES)
	  return;

	do {
		statusMsg.m_type = DEV_STATUS;
		if ((r = sendRec(msg->m_source, &statusMsg)) != OK)
		  panic(__FILE__, "couldn't sendRec for DEV_STATUS", r);

		switch (statusMsg.m_type) {
			case DEV_REVIVE:
				revive(statusMsg.REP_PROC_NR, statusMsg.REP_STATUS);
				break;
			case DEV_IO_READY:
				selectNotified(i, statusMsg.DEV_MINOR, statusMsg.DEV_SEL_OPS);
				break;
			default:
				printf("FS: unrecognized reply %d to DEV_STATUS\n", statusMsg.m_type);
				/* Fall through */
			case DEV_NO_STATUS:
				getMore = false;
				break;
		}
	} while (getMore);
}

int doIoctl() {
/* Perform the ioctl(fd, request, argx) system call. */
	Filp *filp;
	register Inode *rip;
	dev_t dev;

	if ((filp = getFilp(inMsg.m_ls_fd)) == NIL_FILP)
	  return errCode;
	rip = filp->filp_inode;	/* Get inode pointer */
	if ((rip->i_mode & I_TYPE) != I_CHAR_SPECIAL &&
		(rip->i_mode & I_TYPE) != I_BLOCK_SPECIAL)
	  return ENOTTY;
	dev = (dev_t) rip->i_zone[0];

	return devIO(DEV_IOCTL, dev, who, inMsg.ADDRESS, 0L,
				inMsg.REQUEST, filp->filp_flags);
}




