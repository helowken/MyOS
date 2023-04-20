

#include "stdlib.h"
#include "unistd.h"
#include "shell.h"
#include "main.h"
#include "options.h"
#include "output.h"
#include "error.h"
#include "signal.h"
#include "stdarg.h"
#include "errno.h"

/* Code to handle exceptions in C.
 */
JmpLoc *handler;
int exception;
volatile int suppressInt;
volatile int intPending;
char *commandName;


/* Called to raise an exception. Since C doesn't include exception, we
 * just do a longjmp to the exception handler. The type of exception is
 * stored in the global variable "exception"
 */
void exRaise(int e) {
	if (handler == NULL)
	  abort();
	exception = e;
	longjmp(handler->loc, 1);
}

/* Called from trap.c when a SIGINT is received. (If the user specifies
 * that SIGINT is to be trapped or ignored using the trap builtin, then
 * this routine is not called.) SuppressInt is nonzero when interrupts
 * are held using the INTOFF macro. The call to _exit is necessary because
 * there is a short period after a fork before the signal handlers are
 * set to the appropriate value for the child. (The test for iflag is
 * just defensive programming.)
 */
void onInt() {
	if (suppressInt) {
		++intPending;
		return;
	}
	intPending = 0;
	if (rootShell && iflag) 
	  exRaise(EX_INT);
	else
	  _exit(128 + SIGINT);
}

/* Error is called to raise the error exception. If the first argument
 * is not NULL then error prints an error message using printf style
 * formatting. It then raises the error exception.
 */
void error(char *msg, ...) {
	va_list ap;

	CLEAR_PENDING_INT;
	INTOFF;
	va_start(ap, msg);
	if (msg) {
		if (commandName)
		  outFormat(&errOut, "%s: ", commandName);
		doFormat(&errOut, msg, ap);
		out2Char('\n');
	}
	va_end(ap);
	printf("=== TODO error() raise error\n");
	flushAll();
	//TODO exRaise(EX_ERROR);
}

void error2(char *a, char *b) {
	error("%s: %s", a, b);
}

