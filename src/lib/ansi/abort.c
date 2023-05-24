#if defined(_POSIX_SOURCE)
#include <sys/types.h>
#endif
#include <signal.h>
#include <stdlib.h>

extern void (*_clean)();

void abort() {
	if(_clean)
	  _clean();		/* Flush all output files */
	raise(SIGABRT);
}
