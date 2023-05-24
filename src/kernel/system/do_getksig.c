#include "../system.h"
#include <signal.h>

int doGetKernelSig(Message *msg) {
/* PM is ready to accept signals and repeatedly does a kernel call to get
 * one. Find a process with pending signals. If no signals are available,
 * return NONE in the process number field.
 * It is not sufficient to ready the process when PM is informed, because
 * PM can block waiting for FS to do a core dump.
 */
	register Proc *rp;
	for (rp = BEG_USER_ADDR; rp < END_PROC_ADDR; ++rp) {
		if (rp->p_rts_flags & SIGNALED) {
			msg->SIG_PROC = rp->p_nr;		/* Store signaled process */
			msg->SIG_MAP = rp->p_pending;	/* Pending signals map */
			sigemptyset(&rp->p_pending);	/* Ball is in PM's court */
			rp->p_rts_flags &= ~SIGNALED;	/* Blocked by SIG_PENDING */
			return OK;
		}
	}

	/* No process with pending signals was found. */
	msg->SIG_PROC = NONE;
	return OK;
}
