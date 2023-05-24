#include "../system.h" 
#include <string.h>
#include <signal.h>
#include <sys/sigcontext.h>

int doSigReturn(Message *msg) {
/* POSIX style signals require sysSigReturn() to put things in order before
 * the signalled process can resume execution
 */
	struct sigcontext sc;
	register Proc *rp;
	phys_bytes srcPhys;
	int pNum;

	pNum = msg->SIG_PROC;
	if (! isOkProcNum(pNum))
	  return EINVAL;
	if (isKernelNum(pNum))
	  return EPERM;
	rp = procAddr(pNum);

	/* Copy in the sigcontext structure */
	srcPhys = umapLocal(rp, D, (vir_bytes) msg->SIG_CTXT_PTR,
				(vir_bytes) sizeof(struct sigcontext));
	if (srcPhys == 0)
	  return EFAULT;
	physCopy(srcPhys, vir2Phys(&sc), (phys_bytes) sizeof(struct sigcontext));

	/* Make sure that this is not just a jump buffer. */
	if ((sc.sc_flags & SC_SIGCONTEXT) == 0)
	  return EINVAL;

	sc.sc_psw = rp->p_reg.psw;

	/* Don't panic kernel if user gave bad selector. */
	sc.sc_cs = rp->p_reg.cs;
	sc.sc_ds = rp->p_reg.ds;
	sc.sc_es = rp->p_reg.es;
	sc.sc_fs = rp->p_reg.fs;
	sc.sc_gs = rp->p_reg.gs;

	/* Restore the registers. */
	memcpy(&rp->p_reg, &sc.sc_regs, sizeof(struct sigregs));
	return OK;
}
