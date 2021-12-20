#ifndef _SIGCONTEXT_H
#define _SIGCONTEXT_H

#include "minix/sys_config.h"

typedef struct StackFrame	SigRegs;

typedef struct {
	int sc_flags;		/* Sigstack state to restore */
	long sc_mask;		/* Signal mask to restore */
	SigRegs *sc_regs;	/* Register set to restore */
} SigContext;

typedef struct {
	void (*sf_ret_addr)();
	int sf_sig_num;
	int sf_code;
	SigContext *sf_ctx;
	int sf_bp;
	void (*sf_ret_addr2)();
	SigContext *sf_ctx_copy;
} SigFrame;

/* Values for sc_flags. */
#define SC_SIGCONTEXT	2	/* Nonzero when signal context is included */

#endif
