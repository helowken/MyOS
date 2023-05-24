#include "../system.h"
#include <signal.h>
#include <string.h>
#include <sys/sigcontext.h>

/* Note: we assume stack is downwards, then the sigcontext and 
 *       sigframe in stack is as follows:
 *
 *    Proc Stack
 *
 *  | sigcontext #1   |
 *  ------------------- stack frame for _sigreturn
 *  | sf.sf_ctx_copy  | point to sigcontext #1
 *  | sf.sf_ret_addr2 | original PC: rp->p_reg.pc
 *  ------------------- stack frame for sighandler
 *  | sf.sf_bp        | original ebp: rp->p_reg.ebp
 *  | sf.sf_ctx       | sighandler argv[3], point to sigcontext #1
 *  | sf.sf_code      | sighandler argv[2]
 *  | sf.sf_sig_num   | sighandler argv[1]
 *  | sf.sf_ret_addr  | address of _sigreturn
 */

int doSigSend(Message *msg) {
/* Handle sysSigSend, POSIX-style signal handling. */

	SigMsg sigMsg;
	register Proc *rp;
	phys_bytes srcPhys, dstPhys;
	struct sigcontext sigCtx, *sigCtxPtr;
	struct sigframe sigFrame, *sigFramePtr;
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

	/* Compute the user stack pointer where sigcontext will be stored. */
	sigCtxPtr = (struct sigcontext *) sigMsg.sm_stack_ptr - 1;

	/* Copy the registers to the sigcontext structure. */
	memcpy(&sigCtx.sc_regs, (char *) &rp->p_reg, sizeof(struct sigregs));

	/* Finish the sigcontext initialization. */
	sigCtx.sc_flags = SC_SIGCONTEXT;
	sigCtx.sc_mask = sigMsg.sm_mask;

	/* Copy the sigcontext structure to the user's stack. */
	dstPhys = umapLocal(rp, D, (vir_bytes) sigCtxPtr,
				(vir_bytes) sizeof(struct sigcontext));
	if (dstPhys == 0)
	  return EFAULT;
	physCopy(vir2Phys(&sigCtx), dstPhys, (phys_bytes) sizeof(struct sigcontext));

	/* Compute the user stack pointer where sigframe will be stored. */
	sigFramePtr = (struct sigframe *) sigCtxPtr - 1;
	/* Initialize the sigframe structure. */
	sigFrame.sf_ctx_copy = sigCtxPtr;
	sigFrame.sf_ret_addr2 = (void (*)()) rp->p_reg.pc;

	/* The following 2 statements are as: 
	 *  push %ebp			
	 *  movl %esp, %ebp		
	 */
	sigFrame.sf_bp = rp->p_reg.ebp;
	rp->p_reg.ebp = (reg_t) &sigFramePtr->sf_bp;

	sigFrame.sf_ctx = sigCtxPtr;
	sigFrame.sf_code = 0;	/* XXX - should be used for type of FP exception */
	sigFrame.sf_sig_num = sigMsg.sm_sig_num;
	sigFrame.sf_ret_addr = (void (*)()) sigMsg.sm_sig_return;

	/* Copy the sigframe structure to the user's stack. */
	dstPhys = umapLocal(rp, D, (vir_bytes) sigFramePtr,
				(vir_bytes) sizeof(struct sigframe));
	if (dstPhys == 0)
	  return EFAULT;
	physCopy(vir2Phys(&sigFrame), dstPhys, (phys_bytes) sizeof(struct sigframe));

	/* Reset user registers to execute the signal handler. */
	rp->p_reg.esp = (reg_t) sigFramePtr;
	rp->p_reg.pc = (reg_t) sigMsg.sm_sig_handler;

	return OK;
}
