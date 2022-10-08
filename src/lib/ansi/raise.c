#if defined(_POSIX_SOURCE)
#include "sys/types.h"
#endif
#include "signal.h"

extern int kill(int pid, int sig);
extern pid_t getpid();

int raise(int sig) {
	if (sig < 0 || sig > NSIG)
	  return -1;
	return kill(getpid(), sig);
}
