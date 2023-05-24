#include <lib.h>
#include <signal.h>

sighandler_t signal(int sig, sighandler_t disp) {
	struct sigaction sa, osa;

	if (sig <= 0 || sig > NSIG || sig == SIGKILL) {
		errno = EINVAL;
		return SIG_ERR;
	}
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = disp;
	if (sigaction(sig, &sa, &osa) < 0)
	  return SIG_ERR;
	return osa.sa_handler;
}
