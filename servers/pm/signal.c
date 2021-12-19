#include "pm.h"
#include "minix/callnr.h"
#include "minix/com.h"
#include "signal.h"
#include "sys/sigcontext.h"
#include "string.h"
#include "mproc.h"

static void unpause(int pIdx) {
/* A signal is to be sent to a process. If that process is hanging on a 
 * system call, the system call must be terminated with EINTR. Possible
 * calls are PAUSE, WAIT, READ and WRITE, the latter two for pipes and ttys.
 * First check if the process is hanging on an PM call. If not, tell FS,
 * so it can check for READs and WRITEs from pipes, ttys and the like.
 */
	register struct MProc *rmp;
	int flags;

	rmp = &mprocTable[pIdx];
	flags = PAUSED | WAITING | SIGSUSPENDED;

	/* Check to see if process is hanging on a PAUSE, WAIT or SIGSUSPEND call. */
	if (rmp->mp_flags & flags) {
		rmp->mp_flags &= ~flags;
		setReply(pIdx, EINTR);
		return;
	}

	/* Process is not hanging on an PM call. Ask FS to take a look. */
	tellFS(UNPAUSE, pIdx, 0, 0);
}

void signalProc(register MProc *rmp, int sigNum) {
/* Send a signal to a process. Check to see if the signal is to be caught,
 * ignored, transformed into a message (for system processes) or blocked.
 *	- If the signal is to be transformed into a message, request the KERNEL to
 * send the target process a system notification with the pending signal as an
 * argument.
 *	- If the signal is to be caught, request the KERNEL to push a sigcontext
 * structure and a sigframe structure onto the catcher's stack. Also, KERNEL
 * will reset the program counter and stack pointer, so that when the process
 * next runs, it will be executing the signal handler. When the signal handler
 * returns, sigreturn(2) will be called. Then KERNEL will restore the signal
 * context from the sigcontext structure.
 * If there is insufficient stack space, kill the process.
 */
	vir_bytes newSp;
	int s;
	int slot;
	int sigFlags;
	SigMsg sm;
	struct sigaction *sa;

	slot = (int) (rmp - mprocTable);
	if ((rmp->mp_flags & (IN_USE | ZOMBIE)) != IN_USE) {
		printf("PM: signal %d sent to %s process %d\n",
			sigNum, (rmp-mp_flags & ZOMBIE) ? "zombie" : "dead", slot);
		panic(__FILE__, "", NO_NUM);
	}
	if ((rmp->mp_flags & TRACED) && sigNum != SIGKILL) {
		/* A traced process has special handling. */
		unpause(slot);
		stopProc(rmp, sigNum);		/* A signal causes it to stop */
		return;
	}
	/* Some signals are ignored by default. */
	if (sigismember(&rmp->mp_sig_ignore, sigNum)) {
		return;
	}
	if (sigismember($rmp->mp_sig_catch, sigNum)) {
		/* Signal should be blocked. */
		sigaddset(&rmp->mp_sig_pending, sigNum);
		return;
	}
	if (rmp->mp_flags & ONSWAP) {
		/* Process to swapped out, leave signal pending. */
		sigaddset(&rmp->mp_sig_pending, sigNum);
		swapInQueue(rmp);
		return;
	}

	sa = &rmp->mp_sigaction[sigNum];
	sigFlags = sa->sa_flags;
	if (sigismember(&rmp->mp_catch, sigNum)) {
		if (rmp->mp_flags & SIGSUSPENDED)
		  sm.sm_mask = rmp->mp_sig_mask2;
		else
		  sm.sm_mask = rmp->mp_sig_mask;
		sm.sm_sig_num = sigNum;
		sm.sm_sig_handler = (vir_bytes) sa->sa_handler;
		sm.sm_sig_return = rmp->mp_sig_return;
		if ((s = getStackPtr(slot, &newSp)) != OK)
		  panic(__FILE__, "couldn't get new stack pointer", s);
		sm.sm_stack_ptr = newSp;

		// TODO make rootm for newSp and adjust it
		
		rmp->mp_sig_mask |= sa->sa_mask;
		if (sigFlags & SA_NODEFER)
		  sigdelset(&rmp->mp_sig_mask, sigNum);
		else
		  sigaddset(&rmp->mp_sig_mask, sigNum);

		if (sigFlags & SA_RESETHAND) {
			sigdelset(&rmp->mp_catch, sigNum);
			sa->sa_handler = SIG_DFL;
		}

		if ((s == sysSigSend(slot, &sm)) == OK) {
			sigdelset(&rmp>mp_sig_pending, sigNum);
			/* If process is hanging on PAUSE, WAIT, SIGSUSPEND, tty,
			 * pipe, etc., release it.
			 */
			unpause(slot);
			return;
		}
		panic(__FILE__, "wanring, sysSigSend failed", s);
	} else if (sigismember(&rmp->mp_sig_to_msg, sigNum)) {
		if ((s = sysKill(slot, sigNum)) != OK)
		  panic(__FILE__, "warning, sysKill failed", s);
		return;
	}

doTerminate:
	/* Signal should not or cannot be caught. Take default action. */
	if (sigismember(&ignoreSigSet, sigNum))
	  return;

	rmp->mp_sig_status = (char) sigNum;
	if (sigismember(&coreSigSet, sigNum)) {
		if (rmp->mp_flags & ONSWAP) {
			/* Process is swapped out, leave signal pending. */
			sigaddset(&rmp->mp_sig_pending, sigNum);
			swapInQueue(rmp);
			return;
		}

		/* Switch to the user's FS environment and dump core. */
		tellFS(CHDIR, slot, false, 0);
		dumpCore(rmp);
	}
	pmExit(rmp, 0);		/* Terminate process */
}

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
