#include "pm.h"
#include "sys/wait.h"
#include "minix/callnr.h"
#include "minix/com.h"
#include "signal.h"
#include "mproc.h"
#include "param.h"

static void cleanup(register MProc *child) {
/* Finish off the exit of a process. The process has exited or been killed
 * by a signal, and its parent is waiting.
 */
	MProc *parent = &mprocTable[child->mp_parent];
	int exitStatus;

	/* Wake up the parent by sending the reply message. */
	exitStatus = (child->mp_exit_status << 8) | (child->mp_sig_status & 0377);
	parent->mp_reply.reply_res2 = exitStatus;
	setReply(child->mp_parent, child->mp_pid);
	parent->mp_flags &= ~WAITING;		/* Parent no longer waiting */

	/* Release the process table entry and reinitialize some fields. */
	child->mp_pid = 0;
	child->mp_flags = 0;
	child->mp_child_utime = 0;
	child->mp_child_stime = 0;
	--procsInUse;
}

int doPmExit() {
/* Perform the exit(status) system call. The real work is done by pmExit(),
 * which is also called when a process is killed by a signal. 
 */
	pmExit(currMp, inputMsg.status);
	return SUSPEND;		/* Can't communicate from beyond the grave */
}

void pmExit(register MProc *rmp, int exitStatus) {
/* A process is done. Release most of the process' possessions. If its
 * parent is waiting, release the rest, else keep the process slot and
 * become a zombie.
 */
	register int pNum;
	bool parentWaiting, rightChild;
	pid_t waitingPid, procGrp;
	MProc *parentMp, *initMp;
	clock_t t[3];

	pNum = (int) (rmp - mprocTable);	/* Get process slot number */

	/* Remember a session leader's process group. */
	procGrp = (rmp->mp_pid == rmp->mp_proc_grp) ? rmp->mp_proc_grp : 0;

	/* If the exited process has a timer pending, kill it. */
	if (rmp->mp_flags & ALARM_ON)
	  setAlarm(pNum, 0);

	/* Do accounting: fetch usage times and accumulate at parent. */
	sysTimes(pNum, t);
	parentMp = &mprocTable[rmp->mp_parent];
	parentMp->mp_child_utime += t[0] + rmp->mp_child_utime;		/* Add user time */
	parentMp->mp_child_stime += t[1] + rmp->mp_child_stime;		/* Add system time */

	/* Tell the kernel and FS that the process is no longer runnable. */
	tellFS(EXIT, pNum, 0, 0);	/* File system can free the proc slot. */
	sysExit(pNum);

	/* Pending reply messages for the dead process cannot be delivered. */
	rmp->mp_flags &= ~REPLY;

	/* Release the memory ocuupied by the child. */
	if (findShare(rmp, rmp->mp_ino, rmp->mp_dev, rmp->mp_ctime) == NULL) {
		/* No other process shares the text segment, so free it. */
		freeMemory(rmp->mp_memmap[T].physAddr, rmp->mp_memmap[T].len);
	}
	/* Free the data and stack segments. */
	freeMemory(rmp->mp_memmap[D].physAddr, rmp->mp_memmap[S].physAddr + 
				rmp->mp_memmap[S].len - rmp->mp_memmap[D].virAddr);

	/* The process slot can only be freed if the parent has done a WAIT. */
	rmp->mp_exit_status = (char) exitStatus;

	waitingPid = parentMp->mp_wait_pid;		/* Who's being waiting for? */
	parentWaiting = parentMp->mp_flags & WAITING;
	rightChild = (waitingPid == -1 ||		/* Child meets one of the 3 tests? */
				waitingPid == rmp->mp_pid || 
				-waitingPid == rmp->mp_proc_grp);

	if (parentWaiting && rightChild) {
		cleanup(rmp);		/* Tell parent and release child slot */
	} else {
		rmp->mp_flags = IN_USE | ZOMBIE;	/* Parent no waiting, zombify child */
		signalProc(parentMp, SIGCHLD);		/* Send parent a "child died" signal */
	}

	initMp = &mprocTable[INIT_PROC_NR];
	/* If the process has children, disinherit them. INIT is the new parent. */
	for (rmp = &mprocTable[0]; rmp < &mprocTable[NR_PROCS]; ++rmp) {
		if (rmp->mp_flags & IN_USE && rmp->mp_parent == pNum) {
			/* 'rmp' now points to a child to be disinherited. */
			rmp->mp_parent = INIT_PROC_NR;
			parentWaiting = initMp->mp_flags & WAITING;
			if (parentWaiting && (rmp->mp_flags & ZOMBIE))
			  cleanup(rmp);
		}
	}

	/* Send a hangup to the process' process group if it was a session leader. */
	if (procGrp != 0)
	  checkSig(-procGrp, SIGHUP);
}
