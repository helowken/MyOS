#include "../system.h"
#include "sys/ptrace.h"

#define TR_VALUE_SIZE	((vir_bytes) sizeof(long))

int doTrace(Message *msg) {
	register Proc *rp;
	phys_bytes src, dst;
	vir_bytes traceAddr = (vir_bytes) msg->CTL_ADDRESS;
	long traceData = msg->CTL_DATA;
	int traceReq = msg->CTL_REQUEST;
	int tracePNum = msg->CTL_PROC_NR;
	int i, seg = -1;

	if (! isOkProcNum(tracePNum))
	  return EINVAL;
	if (isKernelNum(tracePNum))
	  return EPERM;

	rp = procAddr(tracePNum);
	if (isEmptyProc(rp))
	  return EIO;

	switch (traceReq) {
		case T_STOP:		/* Stop process */
			if (rp->p_rts_flags == 0)
			  lockDequeue(rp);
			rp->p_rts_flags |= P_STOP;
			rp->p_reg.psw &= ~TRACE_BIT;	/* Clear trace bit */
			return OK;
			
		case T_GETINS:		/* Return value from instruction space */
			seg = T;
			/* Fall through */
		case T_GETDATA:		/* Return value from data space */
			if (seg == -1)
			  seg = D;
			if ((src = umapLocal(rp, seg, traceAddr, TR_VALUE_SIZE)) == 0)
			  return EIO;
			physCopy(src, vir2Phys(&traceData), (phys_bytes) TR_VALUE_SIZE);
			msg->CTL_DATA = traceData;
			break;
			
		case T_GETUSER:		/* Return value from process table */
			if ((traceAddr & (TR_VALUE_SIZE - 1)) != 0 ||
				traceAddr > sizeof(Proc) - TR_VALUE_SIZE)
			  return EIO;
			msg->CTL_DATA = *(long *) ((char *) rp + (int) traceAddr);
			break;

		case T_SETINS:		/* Set value in instruction space */
			seg = T;
			/* Fall through */
		case T_SETDATA:		/* Set value in data space */
			if (seg == -1)
			  seg = D;
			if ((dst = umapLocal(rp, seg, traceAddr, TR_VALUE_SIZE)) == 0)
			  return EIO;
			physCopy(vir2Phys(&traceData), dst, (phys_bytes) TR_VALUE_SIZE);
			msg->CTL_DATA = 0;
			break;
			
		case T_SETUSER:		/* Set value in process table */
			if ((traceAddr & (sizeof(reg_t) - 1)) != 0 ||
				traceAddr > sizeof(StackFrame) - sizeof(reg_t))
			  return EIO;
			i = (int) traceAddr;
			/* Altering segment registers might crash the kernel when it
			 * tries to load them prior to restarting a process, so do
			 * not allow it.
			 */
			if (i == (int) &((Proc *) 0)->p_reg.cs ||
				i == (int) &((Proc *) 0)->p_reg.ds ||
				i == (int) &((Proc *) 0)->p_reg.es ||
				i == (int) &((Proc *) 0)->p_reg.fs ||
				i == (int) &((Proc *) 0)->p_reg.gs ||
				i == (int) &((Proc *) 0)->p_reg.ss)
			  return EIO;
			/* Only selected bits are changeable */
			if (i == (int) &((Proc *) 0)->p_reg.psw)	
			  SETPSW(rp, traceAddr);
			else
			  *(reg_t *) ((char *) &rp->p_reg + i) = (reg_t) traceData;
			msg->CTL_DATA = 0;
			break;

		case T_RESUME:		/* Resume execution */
			rp->p_rts_flags &= ~P_STOP;
			if (rp->p_rts_flags == 0)
			  lockEnqueue(rp);
			msg->CTL_DATA = 0;
			break;

		case T_STEP:		/* Set trace bit */
			rp->p_reg.psw |= TRACE_BIT;
			rp->p_rts_flags &= ~P_STOP;
			if (rp->p_rts_flags == 0)
			  lockEnqueue(rp);
			msg->CTL_DATA = 0;
			break;

		default:
			return EIO;
	}
	return OK;
}
