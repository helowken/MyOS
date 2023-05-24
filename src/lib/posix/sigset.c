#include <signal.h>
#include <errno.h>

/* Low bit of signal masks. */
#define SIGBIT_0	((sigset_t) 1)

/* Mask of valid signals (0 ~ NSIG). */
#define SIGMASK		(((SIGBIT_0 << NSIG) << 1) - 1)

#define sigValid(sig)	((unsigned) (sig) <= NSIG)

int sigaddset(sigset_t *set, int sig) {
	if (! sigValid(sig)) {
		errno = EINVAL;
		return -1;
	}
	*set |= SIGBIT_0 << sig;
	return 0;
}

int sigdelset(sigset_t *set, int sig) {
	if (! sigValid(sig)) {
		errno = EINVAL;
		return -1;
	}
	*set &= ~(SIGBIT_0 << sig);
	return 0;
}

int sigemptyset(sigset_t *set) {
	*set = 0;
	return 0;
}

int sigfillset(sigset_t *set) {
	*set = SIGMASK;
	return 0;
}

int sigismember(const sigset_t *set, int sig) {
	if (! sigValid(sig)) {
		errno = EINVAL;
		return -1;
	}
	if (*set & (SIGBIT_0 << sig)) 
	  return 1;
	return 0;
}


