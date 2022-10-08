

#include "shell.h"
#include "redir.h"
#include "eval.h"
#include "memalloc.h"
#include "error.h"
#include "sys/types.h"
#include "signal.h"
#include "fcntl.h"
#include "errno.h"
#include "limits.h"


#define EMPTY	-2			/* Marks an unused slot in RedirectTable */
#define PIPE_SIZE	4096	/* Amount of buffering in a pipe */

/* Copy a file descriptor, like the F_DUPFD option at fcntl. Returns -1
 * if the source file descriptor is closed, EMPTY if there are no unused
 * file descriptors left.
 */
int copyFd(int from, int to) {
	int newFd;

	newFd = fcntl(from, F_DUPFD, to);
	if (newFd < 0 && errno == EMFILE)
	  return EMPTY;
	return newFd;
}
