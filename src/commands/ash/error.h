


/* Types of operations (passed to the errMsg routine). */
#define E_OPEN	01		/* Opening a file */
#define E_CREAT	02		/* Creating a file */
#define E_EXEC	03		/* Executing a program */ 


/* We enclose jmp_buf in a struture so that we can declare pointers to
 * jump locations. The global variable handler contains the location to
 * jump to when an exception occurs, and the global variable exception
 * contains a code identifying the exception. To implement nested
 * exception handlers, the user should save the value of handler on entry
 * to an inner scope, set handler to point to a JmpLoc structure for the
 * inner scope, and restore handler on exit from the scope.
 */
#include "setjmp.h"

typedef struct {
	jmp_buf loc;
} JmpLoc;

extern JmpLoc *handler;
extern int exception;

/* Exceptions */
#define EX_INT			0	/* SIGINT received */
#define EX_ERROR		1	/* A generic error */
#define EX_SHELL_PROC	2	/* Execute a shell procedure */



/* These macros allow the user to suspend the handling of interrupt signals
 * over a period of time. This is Similar to SIGHOLD to or sigblock, but
 * much more efficient and portable. (But hacking the kernel is so much
 * more fun than worrying about efficiency and portability. :-))
 */

extern volatile int suppressInt;
extern volatile int intPending;
extern char *commandName;	/* Name of command--printed on error */

#define INTOFF				++suppressInt
#define INTON				if (--suppressInt == 0 && intPending) onInt(); else
#define FORCE_INTON			{suppressInt = 0; if (intPending) onInt();}
#define CLEAR_PENDING_INT	intPending = 0
#define int_pending()		intPending


void exRaise(int);
void onInt();
void error(char *, ...);
void error2(char *, char *);
char *errMsg(int, int);

#define errMsg(errno, action)	strerror(errno)
