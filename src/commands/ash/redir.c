

#include "shell.h"
#include "redir.h"
#include "eval.h"
#include "memalloc.h"
#include "error.h"
#include "signal.h"
#include "fcntl.h"
#include "errno.h"
#include "limits.h"


#define EMPTY	-2			/* Marks an unused slot in RedirectTable */
#define PIPE_SIZE	4096	/* Amount of buffering in a pipe */


MKINIT
typedef struct RedirTab {
	struct RedirTab *next;
	short renamed[10];
} RedirTab;

MKINIT RedirTab *redirList;


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

/* Undo the effects of the last redirection.
 */
void popRedir() {
	//TODO
}

/* Discard all saved file descriptors.
 */
void clearRedir() {
	//TODO
}

/* Process a list of redirection commands. If the REDIR_PUSH flag is set,
 * old file descriptors are stashed away so that the redirection can be 
 * undone by calling popRedir. If the REDIR_BACK_Q flag is set, then the
 * standard output, and the standard error if it becomes a duplicate of
 * stdout, is saved in memory.
 */
void redirect(union Node *redir, int flags) {
	//TODO
}


/* Undo all redirections. Called on error or interrupt.
 */
#ifdef mkinit
INCLUDE "redir.h"

RESET {
	while (redirList) {
		popRedir();
	}
}

SHELLPROC {
	clearRedir();
}
#endif
