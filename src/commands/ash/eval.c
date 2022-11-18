


#include "shell.h"
#include "nodes.h"
#include "syntax.h"
#include "expand.h"
#include "parser.h"
#include "eval.h"
#include "input.h"
#include "trap.h"
#include "var.h"
#include "options.h"
#include "redir.h"
#include "memalloc.h"
#include "mystring.h"


/* Flags in argument to evalTree */
#define EV_EXIT		01	/* Exit after evaluating tree */
#define EV_TESTED	02	/* Exit status is checked; ignore -e flag */
#define EV_BACKCMD	04	/* Command executing within back quotes */


MKINIT int evalSkip;	/* Set if we are skipping commands */
MKINIT int loopNest;	/* Current loop nesting level */
int funcNest;			/* Depth of function calls */

int exitStatus;


/* Called to reset things after an exception.
 */
#ifdef mkinit
INCLUDE "eval.h"

RESET {
	evalSkip = 0;
	loopNest = 0;
	funcNest = 0;
}

SHELLPROC {
	exitStatus = 0;
}
#endif



/* Execute a command or commands contained in a string.
 */
void evalString(char *s) {
	Node *n;
	StackMark stackMark;

	setStackMark(&stackMark);
	setInputString(s, 1);
	while ((n = parseCmd(0)) != NEOF) {
		evalTree(n, 0);
		popStackMark(&stackMark);
	}
	popFile();
	popStackMark(&stackMark);
}

/* Compute the names of the files in a redirection list. 
 */
static void expRedir(Node *n) {
	register Node *redir;

	for (redir = n; redir; redir = redir->nFile.next) {
		if (redir->type == NFROM ||
				redir->type == NTO ||
				redir->type == NAPPEND) {
			ArgList fn;
			fn.last = &fn.list;
			expandArg(redir->nFile.fileName, &fn, 0);
			redir->nFile.expFileName = fn.list->text;
		}
	}
}

/* Evaluate a parse tree. The value is left in the global variable
 * existStatus.
 */
void evalTree(Node *n, int flags) {
	if (n == NULL) 
	  return;

	switch (n->type) {
		case NSEMI:
			evalTree(n->nBinary.ch1, 0);
			if (evalSkip)
			  goto out;
			evalTree(n->nBinary.ch2, flags);
			break;
		case NAND:
			evalTree(n->nBinary.ch1, EV_TESTED);
			if (evalSkip || exitStatus != 0)
			  goto out;
			evalTree(n->nBinary.ch2, flags);
			break;
		case NOR:
			evalTree(n->nBinary.ch1, EV_TESTED);
			if (evalSkip || exitStatus != 0)
			  goto out;
			evalTree(n->nBinary.ch2, flags);
			break;
		case NREDIR:
			expRedir(n->nRedir.redirect);
			redirect(n->nRedir.redirect, REDIR_PUSH);
			evalTree(n->nRedir.n, flags);
			popRedir();
			break;
			//TODO

	}
out:
	if (pendingSigs)
	  doTrap();
	if ((flags & EV_EXIT) || (eFlag == 1 && exitStatus && !(flags & EV_TESTED)))
	  exitShell(exitStatus);
}




