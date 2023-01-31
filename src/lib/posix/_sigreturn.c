#include "lib.h"
#include "stddef.h"
#include "sys/sigcontext.h"
#include "signal.h"

int sigreturn(register struct sigcontext *sigCtx) {
	sigset_t set;

	/* The message can't be on the stack, because the stack will vanish out
	 * from under us. The send part of sendRec will succeed, but when a
	 * message is sent to restart the current process, who knows what will
	 * be in the place formerly occupied by the message?
	 */
	static Message msg;

	/* Protect against race conditions by blocking all interrupts. */
	sigfillset(&set);
	sigprocmask(SIG_SETMASK, &set, (sigset_t *) NULL);

	msg.m2_l1 = sigCtx->sc_mask;
	msg.m2_i2 = sigCtx->sc_flags;
	msg.m2_p1 = (char *) sigCtx;

	return syscall(MM, SIGRETURN, &msg);	/* Normally this doesn't return */
}
