#include "../system.h"
#include "../ipc.h"
#include "signal.h"

#define FILLED_MASK	(~0)

int doPrivCtl(Message *msg) {
/* Handle sys_privctl(). Update a process' privileges. If the process is not
 * yet a system process, make sure it gets its own privilege structure.
 */
	register Proc *caller;
	register Proc *rp;
	register Priv *sp;
	int pNum;
	int sId;
	int oldFlags;
	int i;

	/* Check whether caller is allowed to make this call. Privileged processes
	 * can only update the privileges of processes that are inhibited from
	 * running by the NO_PRIV flag. This flag is set when a privileged process
	 * forks.
	 */
	caller = procAddr(msg->m_source);
	if (! (priv(caller)->s_flags & SYS_PROC))
	  return EPERM;

	pNum = msg->PR_PROC_NR;
	if (! isOkProcNum(pNum))
	  return EINVAL;

	rp = procAddr(pNum);
	if (! (rp->p_rts_flags & NO_PRIV))
	  return EPERM;

	/* Make sure this process has its own privileges structure. This may fail,
	 * since there are only a limited number of system processes. Then copy the
	 * privileges from the caller and restore some defaults.
	 */
	if ((i = getPriv(rp, SYS_PROC)) != OK)
	  return i;

	sp = priv(rp);
	sId = sp->s_id;		/* Backup privilege id */
	*sp = *priv(caller);		/* Copy from caller */
	sp->s_id = sId;		/* Restore privilege id */
	sp->s_proc_nr = pNum;		/* Reassociate process num */

	/* Remove pending: */
	for (i = 0; i < BITMAP_CHUNKS(NR_SYS_PROCS); ++i) {	
		sp->s_notify_pending.chunk[i] = 0;	/* - notifications */
	}
	sp->s_int_pending = 0;		/* - interrupts */
	sigemptyset(&sp->s_sig_pending);		/* - signals */

	/* Now update the process' privileges as requested. */
	sp->s_trap_mask = FILLED_MASK;
	for (i = 0; i < BITMAP_CHUNKS(NR_SYS_PROCS); ++i) {
		sp->s_ipc_to.chunk[i] = FILLED_MASK;
	}
	unsetSysBit(sp->s_ipc_to, USER_PRIV_ID);
	
	/* All process that this process can send to must be able to reply.
	 * Therefore, their send masks should be updated as well.
	 */
	for (i = 0; i < NR_SYS_PROCS; ++i) {
		if (getSysBit(sp->s_ipc_to, i)) {
			setSysBit(privAddr(i)->s_ipc_to, privId(rp));
		}
	}

	/* Done. Privileges have been set. Allow process to run again. */
	oldFlags = rp->p_rts_flags;
	rp->p_rts_flags &= ~NO_PRIV;
	if (oldFlags != 0 && rp->p_rts_flags == 0)
	  lockEnqueue(rp);

	return OK;
}
