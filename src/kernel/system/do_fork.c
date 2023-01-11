#include "../system.h"
#include "signal.h"
#include "../protect.h"

int doFork(register Message *msg) {
	reg_t oldLdtSel;
	register Proc *childProc;		
	Proc *parentProc;

	parentProc = procAddr(msg->PR_PPROC_NR);
	childProc = procAddr(msg->PR_PROC_NR);
	if (isEmptyProc(parentProc) || ! isEmptyProc(childProc))
	  return EINVAL;

	oldLdtSel = childProc->p_ldt_sel;		/* Backup local descriptors */
	*childProc = *parentProc;				/* Copy 'Proc' struct */
	childProc->p_ldt_sel = oldLdtSel;		/* Restore descriptors */
	childProc->p_nr = msg->PR_PROC_NR;		/* This was obliterated by copy */

	/* Only one in group should have SIGNALED, child doesn't inherit tracing. */
	childProc->p_rts_flags |= NO_MAP;		/* Inhibit process from running. */
	childProc->p_rts_flags &= ~(SIGNALED | SIG_PENDING | P_STOP);
	sigemptyset(&childProc->p_pending);

	/*TODO childProc->p_reg.eax = 0;*/		/* Child sees pid = 0 to know it is child. 
				   (??? It should use setReply(childNum, 0) in pm/forkexit.c) */
	childProc->p_user_time = 0;		/* Set all accounting times to 0. */
	childProc->p_sys_time = 0;

	/* Parent and child have to share the quantum that the forked process had,
	 * so that queued processes do not have to wait longer because of the fork.
	 * If the time left is odd, the child gets an extra tick.
	 */
	childProc->p_ticks_left = (childProc->p_ticks_left + 1) / 2;
	parentProc->p_ticks_left = parentProc->p_ticks_left / 2;

	/* If the parent is a privileged process, take away the privileges from the
	 * child process and inhibit it from running by setting the NO_PRIV flag.
	 * The caller should explicitely set the new privileges before executing.
	 */
	if (priv(parentProc)->s_flags & SYS_PROC) {
		childProc->p_priv = privAddr(USER_PRIV_ID);
		childProc->p_rts_flags |= NO_PRIV;
	}
	return OK;
}
