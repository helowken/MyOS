#include "pm.h"
#include "minix/callnr.h"
#include "minix/com.h"
#include "signal.h"
#include "sys/sigcontext.h"
#include "string.h"
#include "mproc.h"

void checkSig(pid_t pid, int sigNum) {
/* Check to see if it is possible to send a signal. The signal may have to be
 * sent to a group of processes. This routine is invoked by the KILL system
 * call, and also when the kernel catches a DEL or other signal.
 */
	register MProc *rmp;
	int count;		/* Count # of signals sent */
	int errCode;

	if (sigNum < 0 || sigNum > NSIG)
	  return EINVAL;

	/* Return EINVAL for attempts to send SIGKILL to INIT alone. */
	if (pid == INIT_PID && sigNum == SIGKILL)
	  return EINVAL;

	/* Search the proc table for processes to signal. */
	count = 0;
	errCode = ESRCH;
	for (rmp = &mprocTable[0]; rmp < &mprocTable[NR_PROCS]; ++rmp) {
		if ( !(rmp->mp_flags & IN_USE) ||
			((rmp->mp_flags & ZOMBIE) && sigNum != 0) )
		  continue;

		/* Check for selection. */
		if ((pid > 0 && pNum != rmp->mp_pid) ||
			(pid == 0 && currMp->mp_proc_grp != rmp->mp_proc_grp) ||
			(pid == -1 && rmp->mp_pid <= INIT_PID) ||
			(pid < -1 && rmp->mp_proc_grp != -pid))
		  continue;

		/* Check for permission. */
		if (currMp->mp_euid != SUPER_USER &&
			currMp->mp_ruid != rmp->mp_ruid &&
			currMp->mp_euid != rmp->mp_ruid &&
			currMp->mp_ruid != rmp->mp_euid &&
			currMp->mp_euid != rmp->mp_euid) {
			errCode = EPERM;
			continue;
		}

		++count;
		if (sigNum == 0)
		  continue;
		
		/* 'signalProc' will handle the disposition of the signal. The
		 * signal may be caught, blocked, ignored, or cause process
		 * termination, possibly with core dump.
		 */
		signalProc(rmp, sigNum);
	
		if (pid > 0)
		  break;		/* Only one process being signaled */
	}

	/* If the calling process has killed itself, don't reply. */
	if ((currMp->mp_flags & (IN_USE | ZOMBIE)) != IN_USE)
	  return SUSPEND;
	return count > 0 ? OK : errCode;
}

static void handleSig(int pNum, sigset_t sigMap) {
	register MProc *rmp;
	int sig;
	pid_t pid, id;

	rmp = &mprocTable[pNum];
	if ((rmp->mp_flags & (IN_USE | ZOMBIE)) != IN_USE)
	  return;
	pid = rmp->mp_pid;
	currMp = &mprocTable[0];	/* Pretend signals are from PM */
	currMp->mp_proc_grp = rmp->mp_proc_grp;		/* Get process group right */

	/* Check each bit in turn to see if a signal is to be sent. Unlike
	 * kill(), the kernel may collect serveral unrelated signals for a 
	 * process and pass them to PM in one blow. Thus loop on the bit
	 * map. For SIGINT and SIGQUIT, use pid 0 to indicate a broadcast
	 * to the recipient's process group. For SIGKILL, use pid -1 to
	 * indicate a systemwide broadcast.
	 */
	for (sig = 1; sig <= NSIG; ++sig) {
		if (!sigismember(&sigMap, sig))
			continue;
		switch (sig) {
			case SIGINT:
			case SIGQUIT:
				id = 0;		/* Broadcast to process group */
				break;
			case SIGKILL:
				id = -1;	/* Broadcast to all except INIT */
				break;
			default:
				id = pid;
				break;
		}
		checkSig(id, sig);
	}
}

int kernelSigPending() {
/* Certain signals, such as segmentation violations originate in the kernel.
 * When the kernel detects such signals, it notifies the PM to take further
 * action. The PM requests the kernel to send messages with the process
 * slot and bit map for all signaled processes. The File System, for example,
 * uses this mechanism to signal writing on broken pipes (SIGPIPE).
 *
 * The kernel has notified the PM about pending signals. Request pending
 * signals until all signals are handled. If there are no more signals,
 * NONE is returned in the process number field.
 */
	int pNum;
	sigset_t sigMap;

	while (true) {
		sysGetKernelSig(&pNum, &sigMap);	/* Get an arbitrary pending signal */
		if (NONE == pNum) {
			break;
		} else {
			handleSig(pNum, sigMap);	/* Handle the received signal */
			sysEndKernelSig(pNum);		/* Tell kernel it's done */
		}
	}
	return SUSPEND;
}
