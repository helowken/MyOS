#include "../system.h"
#include "signal.h"

int doKill(Message *msg) {
/* Handle sys_kill(). Cause a signal to be sent to a process. The PM is the
 * central server where all signals are processed and handler policies can
 * be registered. Any request, except for PM requests, is added to the map
 * of pending signals and the PM is informed about the new signal.
 * Since system servers cannot use normal POSIX signal handlers (because they
 * are usually blocked on a RECEIVE), they can request the PM to transform
 * signals into messages. This is done by the PM with a call to sys_kill().
 */
	proc_nr_t pNum = msg->SIG_PROC;
	int sig = msg->SIG_NUMBER;

	if (! isOkProcNum(pNum) || sig > NSIG)
	  return EINVAL;
	if (isKernelNum(pNum))
	  return EPERM;

	if (msg->m_source == PM_PROC_NR) {
		/* Directly send signal notification to a system process. */
		if (! (priv(procAddr(pNum))->s_flags & SYS_PROC))
		  return EPERM;
		sendSig(pNum, sig);
	} else {
		/* Set pending signal to be processed by the PM. */
		causeSig(pNum, sig);
	}
	return OK;
}
