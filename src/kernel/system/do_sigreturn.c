#include "../system.h" 
#include "string.h"
#include "signal.h"
#include "sys/sigcontext.h"

int doSigReturn(Message *msg) {
/* POSIX style signals require sysSigReturn() to put things in order before
 * the signalled process can resume execution
 */
	SigContext sigCtx;
	register Proc *rp;
	phys_bytes srcPhys;
	int pNum;

	pNum = msg->SIG_PROC;
	if (! isOkProcNum(pNum))
	  return EINVAL;
	if (isKernelNum(pNum))
	  return EPERM;
	rp = procAddr(pNum);

	/* Copy in the SigContext structure */
	srcPhys = umapLocal(rp, D, (vir_bytes) msg->SIG_CTXT_PTR,
				(vir_bytes) sizeof(SigContext));
	if (srcPhys == 0)
	  return EFAULT;
	physCopy(srcPhys, vir2Phys(&sigCtx), (phys_bytes) sizeof(SigContext));

	/* Make sure that this is not just a jump buffer. */
	if ((sigCtx.sc_flags & SC_SIGCONTEXT) == 0)
	  return EINVAL;

	sigCtx.sc_regs.psw = rp->p_reg.psw;

	/* Don't panic kernel if user gave bad selector. */
	sigCtx.sc_regs.cs = rp->p_reg.cs;
	sigCtx.sc_regs.ds = rp->p_reg.ds;
	sigCtx.sc_regs.es = rp->p_reg.es;
	sigCtx.sc_regs.fs = rp->p_reg.fs;
	sigCtx.sc_regs.gs = rp->p_reg.gs;

	/* Restore the registers. */
	memcpy(&rp->p_reg, &sigCtx.sc_regs, sizeof(SigRegs));
	return OK;
}
