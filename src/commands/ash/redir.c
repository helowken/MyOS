

#include "unistd.h"
#include "shell.h"
#include "nodes.h"
#include "redir.h"
#include "eval.h"
#include "memalloc.h"
#include "error.h"
#include "signal.h"
#include "fcntl.h"
#include "errno.h"
#include "output.h"
#include "limits.h"
#include "fcntl.h"


#define EMPTY		-2		/* Marks an unused slot in RedirTable */
#define PIPE_SIZE	4096	/* Amount of buffering in a pipe */


MKINIT
typedef struct RedirTable {
	struct RedirTable *next;
	short renamed[10];
} RedirTable;

MKINIT RedirTable *redirList;

/* We keep track of whether or not fd0 has been redirected. This is for
 * background commands, where we want to redirect fd0 to /dev/null only
 * if it hasn't already been redirected. */
static int fd0Redirected = 0;


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
	register RedirTable *rp = redirList;
	int i;

	for (i = 0; i < 10; ++i) {
		if (rp->renamed[i] != EMPTY) {
			if (i == 0)
			  --fd0Redirected;
			close(i);
			if (rp->renamed[i] >= 0) {
				copyFd(rp->renamed[i], i);
				close(rp->renamed[i]);
			}
		}
	}
	INTOFF;
	redirList = rp->next;
	ckFree(rp);
	INTON;
}

/* Discard all saved file descriptors.
 */
void clearRedir() {
	//TODO
}

static void openRedirect(Node *redir, char memory[10]) {
	int fd = redir->nFile.fd;
	char *fileName;
	int f;

	/* Assume redirection succeeds. */
	exitStatus = 0;

	/* We suppress interrupts so that we won't leave open file
	 * descriptors around. This may not be such a good idea because
	 * an open of a device or a fifo can block indefinitely.
	 */
	//TODO
}

/* Process a list of redirection commands. If the REDIR_PUSH flag is set,
 * old file descriptors are stashed away so that the redirection can be 
 * undone by calling popRedir. If the REDIR_BACK_Q flag is set, then the
 * standard output, and the standard error if it becomes a duplicate of
 * stdout, is saved in memory.
 */
void redirect(union Node *redir, int flags) {
	Node *n;
	RedirTable *sv;
	int i;
	int fd;
	char memory[10];	/* File descriptors to write to memory */

	for (i = 10; --i >= 0; ) {
		memory[i] = 0;
	}
	memory[1] = flags & REDIR_BACK_Q;
	if (flags & REDIR_PUSH) {
		sv = ckMalloc(sizeof(RedirTable));
		for (i = 0; i < 10; ++i) {
			sv->renamed[i] = EMPTY;
		}
		sv->next = redirList;
		redirList = sv;
	}
	for (n = redir; n; n = n->nFile.next) {
		fd = n->nFile.fd;
		if ((flags & REDIR_PUSH) && sv->renamed[fd] == EMPTY) {
			INTOFF;
			if ((i = copyFd(fd, 10)) != EMPTY) {
				sv->renamed[fd] = i;
				close(fd);
			}
			INTON;
			if (i == EMPTY)
			  error("OUt of file descriptors");
		} else {
			close(fd);
		}
		if (fd == 0)
		  ++fd0Redirected;
		openRedirect(n, memory);
	}
	if (memory[1])
	  out1 = &memOut;
	if (memory[2])
	  out2 = &memOut;
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
