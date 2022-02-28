#include "fs.h"
#include "fcntl.h"
#include "minix/callnr.h"
#include "minix/com.h"
#include "file.h"
#include "fproc.h"
#include "param.h"

int noDev(int op, dev_t dev, int proc, int flags) {
	return ENODEV;
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

	return msg.RESP_STATUS;
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
		if (msg->m_type == CANCEL && recMsg.RESP_PROC_NR == pNum) 
		  return;

		/* Otherwise it should be a REVIVE. */
		if (recMsg.m_type != REVIVE) {
			printf("FS: strange device reply from %d, type = %d, proc = %d (1)\n",
					recMsg.m_source, recMsg.m_type, recMsg.RESP_PROC_NR);
			continue;
		}

		revive(recMsg.RESP_PROC_NR, recMsg.RESP_STATUS);
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
		if (msg->RESP_PROC_NR == pNum) {
			break;
		} else if (msg->m_type == REVIVE) {
			/* Otherwise it should be a REVIVE. */
			revive(msg->RESP_PROC_NR, msg->RESP_STATUS);
		} else {
			printf("FS: strange device reply from %d, type = %d, proc = %d (1)\n",
					msg->m_source, msg->m_type, msg->RESP_PROC_NR);
			return;
		}

		r = receive(taskNum, msg);
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
	if (msg.RESP_STATUS == SUSPEND) {
		if (flags & O_NONBLOCK) {
			/* Not supposed to block. */
			msg.m_type = CANCEL;
			msg.PROC_NR = proc;
			msg.DEVICE = minorDev(dev);
			(*dp->dmap_io)(dp->dmap_driver, &msg);
			if (msg.RESP_STATUS == EINTR)
			  msg.RESP_STATUS = EAGAIN;
		} else {
			/* Suspend user. */
			suspend(dp->dmap_driver);
			return SUSPEND;
		}
	}
	return msg.RESP_STATUS;
}
