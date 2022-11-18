#if defined(_POSIX_SOURCE)

#include "sys/types.h"
#include "signal.h"
#include "stddef.h"

void __newSigset(sigset_t *p) {
	/* The SIG_SETMASK is not significant */
	sigprocmask(SIG_SETMASK, NULL, p);
}

void __oldSigset(sigset_t *p) {
	sigprocmask(SIG_SETMASK, p, NULL);
}

#endif
