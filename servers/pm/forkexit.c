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

int doWaitPid() {
/* A process wants to wait for a child to terminate. If a child is already
 * waiting, go clean it up and let this WAIT call terminate. Otherwise,
 * really wait.
 * A process calling WAIT never gets a reply in the usual way at the end
 * of the main loop (unless WNOHANG is set or no qualifying child exists).
 * If a child has already exited, the routine cleanup() sends the reply
 * to awaken the caller.
 * Both WAIT and WAITPID are handled by this code.
 */
	register MProc *rmp;
	int pid, options, children;

	pid = (callNum == WAIT ? -1 : inputMsg.proc_id);	/* 1st param of waitpid */
	options = (callNum == WAIT ? 0 : inputMsg.sig_num);	/* 3rd param of waitpid */
	if (pid == 0)
	  pid = -currMp->mp_proc_grp;

	/* Is there a child waiting to be collected? At this point, pid != 0:
	 *	  pid  >  0 means pid is pid of a specific process to wait for
	 *	  pid == -1 means wait for any child
	 *	  pid  < -1 means wait for any child whose process group = -pid
	 */
	children = 0;
	for (rmp = &mprocTable[0]; rmp < &mprocTable[NR_PROCS]; ++rmp) {
		if ((rmp->mp_flags & IN_USE) && rmp->mp_parent == who) {
			/* The value of pid determines which children qualify. */
			if (pid > 0 && pid != rmp->mp_pid)
			  continue;
			if (pid < -1 && -pid != rmp->mp_proc_grp)
			  continue;

			++children;		/* This child is acceptable */
			if (rmp->mp_flags & ZOMBIE) {
				/* This child meets the pid test and has exited. */
				cleanup(rmp);	
				return SUSPEND;
			} 
			if ((rmp->mp_flags & STOPPED) && rmp->mp_sig_status) {
				/* This child meets the pid test and is being traced. */
				currMp->mp_reply.reply_res2 = 0177 | (rmp->mp_sig_status << 8);
				rmp->mp_sig_status = 0;
				return rmp->mp_pid;
			}
		}
	}

	/* No qualifying child has exited. Wait for one, unless none exists. */
	if (children > 0) {
		/* At least 1 child meets the pid test exists, but has not exited. */
		if (options & WNOHANG)
		  return 0;		/* Parent does not want to wait. */
		currMp->mp_flags |= WAITING;		/* Parent wants to wait */
		currMp->mp_wait_pid = (pid_t) pid;	/* Save pid for later */
		return SUSPEND;
	} else {
		/* No child even meets the pid test. Return error immediately. */
		return ECHILD;
	}
}
