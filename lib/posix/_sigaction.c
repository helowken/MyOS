#include "lib.h"
#define sigaction	_sigaction
#include "sys/sigcontext.h"
#include "signal.h"

int _sigReturn();

int sigaction(int sig, const struct sigaction *sa, struct sigaction *oldSa) {
	Message msg;

	msg.sig_num = sig;
	msg.sig_new_sa = sa;
	msg.sig_old_sa = oldSa;
	msg.sig_return = _sigReturn;

	return syscall(MM, SIGACTION, &msg);
}

