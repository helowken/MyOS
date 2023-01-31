#ifndef _SIGCONTEXT_H
#define _SIGCONTEXT_H

#include "minix/sys_config.h"

/* The following structure should match the StackFrame structure used
 * by the kernel's context switching code. Floating point registers should
 * be added in a different struct.
 */
struct sigregs {
	int sr_gs;
	int sr_fs;
	int sr_es;
	int sr_ds;
	int sr_edi;
	int sr_esi;
	int sr_ebp;
	int sr_temp;		/* Stack top -- used in kernel */
	int sr_ebx;
	int sr_edx;
	int sr_ecx;
	int sr_eax;
	int sr_retaddr;		/* Return address to caller of save -- used in kernel */
	
	int sr_pc;
	int sr_cs;
	int sr_psw;
	int sr_esp;
	int sr_ss;
};

struct sigcontext {
	int sc_flags;			/* Sigstack state to restore */
	long sc_mask;			/* Signal mask to restore */
	struct sigregs sc_regs;	/* Register set to restore */
};

struct sigframe {
	void (*sf_ret_addr)();
	int sf_sig_num;
	int sf_code;
	struct sigcontext *sf_ctx;
	int sf_bp;
	void (*sf_ret_addr2)();
	struct sigcontext *sf_ctx_copy;
};


#define sc_gs		sc_regs.sr_gs
#define sc_fs		sc_regs.sr_fs
#define sc_es		sc_regs.sr_es
#define sc_ds		sc_regs.sr_ds
#define sc_edi		sc_regs.sr_edi
#define sc_esi		sc_regs.sr_esi
#define sc_ebp		sc_regs.sr_ebp
#define sc_temp		sc_regs.sr_temp		/* Stack top -- used in kernel */
#define sc_ebx		sc_regs.sr_ebx
#define sc_edx		sc_regs.sr_edx
#define sc_ecx		sc_regs.sr_ecx
#define sc_eax		sc_regs.sr_eax
#define sc_retaddr	sc_regs.sr_retaddr	/* Return address to caller of save -- used in kernel */

#define sc_pc		sc_regs.sr_pc
#define sc_cs		sc_regs.sr_cs
#define sc_psw		sc_regs.sr_psw
#define sc_esp		sc_regs.sr_esp
#define sc_ss		sc_regs.sr_ss


/* Values for sc_flags. */
#define SC_SIGCONTEXT	2	/* Nonzero when signal context is included */

int sigreturn(struct sigcontext *sigCtx);

#endif
