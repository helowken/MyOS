#include "../system.h"

int doEndKernelSig(Message *msg) {
/* Finish up after a kernel type signal, caused by a SYS_KILL message or a
 * call to causeSig by a task. This is called by the PM after processing a
 * signal it got with SYS_GETKSIG.
 */
	register Proc *rp;

	/* Get process pointer and verify that it had signals pending. If the
	 * process is already dead its flags will be reset.
	 */
	rp = procAddr(msg->SIG_PROC);
	if (! (rp->p_rt_flags & SIG_PENDING))
	  return EINVAL;

	/* PM has finished one kernel signal. Perhaps process is ready now? */
	if (! (rp->p_rt_flags & SIGNALED)) {		/* New signal arrived? */
		if ((rp->p_rt_flags &= ~SIG_PENDING) == 0)
		  lockEnqueue(rp);
	}
	return OK;
}
