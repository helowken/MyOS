

#include "unistd.h"
#include "stdlib.h"
#include "shell.h"
#include "nodes.h"
#include "redir.h"
#include "eval.h"
#include "memalloc.h"
#include "mystring.h"
#include "error.h"
#include "signal.h"
#include "fcntl.h"
#include "errno.h"
#include "output.h"
#include "limits.h"
#include "fcntl.h"


#define EMPTY		-2		/* Marks an unused slot in RedirTable */
#define PIPE_SIZE	4096	/* Amount of buffering in a pipe */
#define RENAME_SIZE	10


MKINIT
typedef struct RedirTable {
	struct RedirTable *next;
	short renamed[RENAME_SIZE];
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

	for (i = 0; i < RENAME_SIZE; ++i) {
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
	register RedirTable *rp;
	int i;

	for (rp = redirList; rp; rp = rp->next) {
		for (i = 0; i < RENAME_SIZE; ++i) {
			if (rp->renamed[i] >= 0) 
			  close(rp->renamed[i]);
			rp->renamed[i] = EMPTY;
		}
	}
}

static void openRedirect(Node *redir, char memory[RENAME_SIZE]) {
	int fd = redir->nFile.fd;
	char *fileName;
	int f;

	/* Assume redirection succeeds. */
	exitStatus = 0;

	/* We suppress interrupts so that we won't leave open file
	 * descriptors around. This may not be such a good idea because
	 * an open of a device or a fifo can block indefinitely.
	 */
	INTOFF;
	memory[fd] = 0;
	switch(redir->nFile.type) {
		case NFROM:
			fileName = redir->nFile.expFileName;
			if ((f = open(fileName, O_RDONLY)) < 0)
			  error("cannot open %s: %s", fileName, errMsg(errno, E_OPEN));
moveFd:
			if (f != fd) {
				copyFd(f, fd);
				close(f);
			}
			break;
		case NTO:
			fileName = redir->nFile.expFileName;
			if ((f = creat(fileName, 0666)) < 0)
			  error("cannot create %s: %s", fileName, errMsg(errno, E_CREAT));
			goto moveFd;
		case NAPPEND:
			fileName = redir->nFile.expFileName;
			if ((f = open(fileName, O_WRONLY | O_CREAT | O_APPEND, 0666)) < 0)
			  error("cannot create %s: %s", fileName, errMsg(errno, E_CREAT));
			goto moveFd;
		case NTOFD:
		case NFROMFD:
			if (redir->nDup.dupFd >= 0) {	/* If not ">&-" */
				if (memory[redir->nDup.dupFd])
				  memory[fd] = 1;
				else
				  copyFd(redir->nDup.dupFd, fd);
			}
			break;
		case NHERE:
		case NXHERE:
			//TODO f = openHere(redir);
			goto moveFd;
		default:
			abort();
	}
	INTON;
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
	char memory[RENAME_SIZE];	/* File descriptors to write to memory */

	for (i = RENAME_SIZE; --i >= 0; ) {
		memory[i] = 0;
	}
	memory[1] = flags & REDIR_BACK_Q;
	if (flags & REDIR_PUSH) {
		sv = ckMalloc(sizeof(RedirTable));
		for (i = 0; i < RENAME_SIZE; ++i) {
			sv->renamed[i] = EMPTY;
		}
		sv->next = redirList;
		redirList = sv;
	}
	for (n = redir; n; n = n->nFile.next) {
		fd = n->nFile.fd;
		if ((flags & REDIR_PUSH) && sv->renamed[fd] == EMPTY) {
			INTOFF;
			/* Dup fd, start from RENAME_SIZE, then mark the mapping */
			if ((i = copyFd(fd, RENAME_SIZE)) != EMPTY) {
				sv->renamed[fd] = i;
				close(fd);
			}
			INTON;
			if (i == EMPTY) 
			  error("Out of file descriptors");
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

/* Return true if fd 0 has already been redirected at least once. */
int isFd0Redirected() {
	return fd0Redirected != 0;
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
