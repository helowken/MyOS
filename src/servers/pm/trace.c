#include "pm.h"
#include "minix/com.h"
#include "sys/ptrace.h"
#include "signal.h"
#include "mproc.h"
#include "param.h"

#define	NIL_MPROC	((MProc *) 0)

void stopProc(register MProc *rmp, int sigNum) {
/* A traced process got a signal so stop it. */
	register MProc *parentMp = mprocTable + rmp->mp_parent;

	if (sysTrace(-1, (int) (rmp - mprocTable), 0L, (long *) 0) != OK)
	  return;
	rmp->mp_flags |= STOPPED;
	if (parentMp->mp_flags & WAITING) {
		parentMp->mp_flags &= ~WAITING;	/* Parent is no longer waiting */
		parentMp->mp_reply.m_reply_res2 = 0177 | (sigNum << 8);
		setReply(rmp->mp_parent, rmp->mp_pid);
	} else {
		rmp->mp_sig_status = sigNum;
	}
}
