#include "../system.h"
#include "string.h"

int doExec(register Message *msg) {
	register Proc *rp;
	reg_t sp;		/* New sp */
	phys_bytes physName;
	char *np;

	rp = procAddr(msg->PR_PROC_NR);
	sp = (reg_t) msg->PR_STACK_PTR;
	rp->p_reg.esp = sp;		/* Set the stack pointer. */

	physMemset(vir2Phys(&rp->p_ldt[EXTRA_LDT_INDEX]), 0,
		(LDT_SIZE - EXTRA_LDT_INDEX) * sizeof(rp->p_ldt[0]));

	rp->p_reg.pc = (reg_t) msg->PR_IP_PTR;	/* Set pc */
	rp->p_rt_flags &= ~RECEIVING;	/* PM does not reply to EXEC call */
	if (rp->p_rt_flags == 0)
	  lockEnqueue(rp);

	/* Save command name for debugging, ps(1) output, etc. */
	physName = nUmapLocal(msg->m_source, (vir_bytes) msg->PR_NAME_PTR,
					(vir_bytes) P_NAME_LEN - 1);
	if (physName != 0) {
		physCopy(physName, vir2Phys(rp->p_name), (phys_bytes) P_NAME_LEN - 1);
		for (np = rp->p_name; (*np & BYTE) >= ' '; ++np) {
		}
		*np = 0;	/* Mark end. */
	} else {
		strncpy(rp->p_name, "<unset>", P_NAME_LEN);
	}
	return OK;
}
