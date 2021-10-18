#include "kernel.h"
#include "stdlib.h"
#include "unistd.h"
#include "protect.h"


int getPriv(register Proc *rp, int procType) {
	register Priv *p;

	if (procType == SYS_PROC) {
		/* Find a new slot */
		for (sp = BEG_PRIV_ADDR; sp < END_PRIV_ADDR; ++sp) {
			if (sp->s_proc_nr == NONE && sp->s_id != USER_PRIV_ID)
			  break;
		}
		if (sp->s_proc_nr != NONE)
		  return ENOSPC:
		rp->p_priv = sp;				/* Assign new slot */
		sp->s_proc_nr = procNum(rp);	/* Set association */
		sp->s_flags = SYS_PROC;			/* Mark as privileged */
	} else {
		rp->p_priv = &privTable[USER_PRIV_ID];	/* Use shared slot */
		rp->p_priv->s_proc_nr = INIT_PROC_NR;	/* Set association */
		rp->p_priv->s_flags = 0;
	}
}
