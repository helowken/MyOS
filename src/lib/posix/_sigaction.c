#include <lib.h>
#include <sys/sigcontext.h>
#include <signal.h>

int _sigreturn();

int sigaction(int sig, const struct sigaction *sa, struct sigaction *oldSa) {
	Message msg;

	msg.m1_i2 = sig;
	msg.m1_p1 = (char *) sa;
	msg.m1_p2 = (char *) oldSa;
	msg.m1_p3 = (char *) _sigreturn;

	return syscall(MM, SIGACTION, &msg);
}

