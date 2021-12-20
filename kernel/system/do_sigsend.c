#include "../system.h"
#include "signal.h"
#include "string.h"
#include "sys/sigcontext.h"

int doSigSend(Message *msg) {
/* Handle sysSigSend, POSIX-style signal handling. */

	SigMsg sigMsg;
	register Proc *rp;
	phys_bytes srcPhys, dstPhys;
	SigContext sigCtx, *sigCtxPtr;
	SigFrame sigFrame, *sigFramePtr;
	int pNum;

	pNum = msg->SIG_PROC;
	if (! isOkProcNum(pNum))
	  return EINVAL;
	if (isKernelNum(pNum))
	  return EPERM;
	rp = procAddr(pNum);

	/* Get the SigMsg structure into our address space. */
	srcPhys = umapLocal(procAddr(PM_PROC_NR), D, (vir_bytes) msg->SIG_CTXT_PTR, 
				(vir_bytes) sizeof(SigMsg));
	if (srcPhys == 0)
	  return EFAULT;
	physCopy(srcPhys, vir2Phys(&sigMsg), (phys_bytes) sizeof(SigMsg));

	/* Compute the user stack pointer where SigContext will be stored. */
	sigCtxPtr = (SigContext *) sigMsg.sm_stack_ptr - 1;

	/* Copy the registers to the SigContext structure. */
	memcpy(&sigCtx.sc_regs, (char *) &rp->p_reg, sizeof(SigRegs));

	/* Finish the SigContext initialization. */
	sigCtx.sc_flags = SC_SIGCONTEXT;
	sigCtx.sc_mask = sigMsg.sm_mask;

	/* Copy the SigContext structure to the user's stack. */
	dstPhys = umapLocal(rp, D, (vir_bytes) sigCtxPtr,
				(vir_bytes) sizeof(SigContext));
	if (dstPhys == 0)
	  return EFAULT;
	physCopy(vir2Phys(&sigCtx), dstPhys, (phys_bytes) sizeof(SigContext));

	/* Initialize the SigFrame structure. */
	sigFramePtr = (SigFrame *) sigCtxPtr - 1;
	sigFrame.sf_ctx_copy = sigCtxPtr;
	sigFrame.sf_ret_addr2 = (void (*)()) rp->p_reg.pc;
	sigFrame.sf_bp = rp->p_reg.ebp;
	rp->p_reg.ebp = (reg_t) &sigFramePtr->sf_bp;
	sigFrame.sf_ctx = sigCtxPtr;
	sigFrame.sf_code = 0;	/* XXX - should be used for type of FP exception */
	sigFrame.sf_sig_num = sigMsg.sm_sig_num;
	sigFrame.sf_ret_addr = (void (*)()) sigMsg.sm_sig_return;

	/* Copy the SigFrame structure to the user's stack. */
	dstPhys = umapLocal(rp, D, (vir_bytes) sigFramePtr,
				(vir_bytes) sizeof(SigFrame));
	if (dstPhys == 0)
	  return EFAULT;
	physCopy(vir2Phys(&sigFrame), dstPhys, (phys_bytes) sizeof(SigFrame));

	/* Reset user registers to execute the signal handler. */
	rp->p_reg.esp = (reg_t) sigFramePtr;
	rp->p_reg.pc = (reg_t) sigMsg.sm_sig_handler;

	return OK;
}
