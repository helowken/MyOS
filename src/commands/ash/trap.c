


#include "unistd.h"
#include "shell.h"
#include "main.h"
#include "eval.h"
#include "options.h"
#include "syntax.h"
#include "signames.h"
#include "memalloc.h"
#include "error.h"
#include "trap.h"
#include "mystring.h"
#include "signal.h"


/* Sigmode records the current value of the signal handlers for the various
 * modes. A value of zero means that the current handler is not known.
 * S_HARD_IGN indicates that the signal was ignored on entry to the shell.
 */

#define S_DFL	1		/* Default signal handling (SIG_DFL) */
#define S_CATCH	2		/* Signal is caught */
#define S_IGN	3		/* Signal is ignored (SIG_IGN) */
#define S_HARD_IGN	4	/* Signal is ignored permenatly */

char *trap[MAX_SIG + 1];	/* Trap handler commands */
MKINIT char sigMode[MAX_SIG];	/* Current value of signal */
char gotSig[MAX_SIG];	/* Indicates specified signal received */
int pendingSigs;		/* Indicates some signal received */

/* Controls whether the shell is interactive or not.
 */
static int isInteractive;

void setInteractive(int on) {
	if (on == isInteractive)
	  return;
	setSignal(SIGINT);
	setSignal(SIGQUIT);
	setSignal(SIGTERM);
	isInteractive = on;
}

/* Signal handler.
 */
static void onSig(int sigNum) {
	signal(sigNum, onSig);
	if (sigNum == SIGINT && trap[SIGINT] == NULL) {
		onInt();
		return;
	}
	gotSig[sigNum - 1] = 1;
	++pendingSigs;
}

/* Called to execute a trap. Perhaps we should avoid entering new trap
 * handlers while we are executing a trap handler.
 */
void doTrap() {
	int i;
	int saveStatus;

	for (;;) {
		for (i = 1; ; ++i) {
			if (gotSig[i - 1])	
			  break;
			if (i >= MAX_SIG)
			  goto done;
		}
		gotSig[i - 1] = 0;
		saveStatus = exitStatus;
		evalString(trap[i]);
		exitStatus = saveStatus;
	}
done:
	pendingSigs = 0;
}

/* Set the signal handler for the specified signal. The routine figures
 * out what it should be set to.
 */
int setSignal(int sigNum) {
	int action;
	sig_t sigAct;
	char *t;

	if ((t = trap[sigNum]) == NULL)
	  action = S_DFL;
	else if (*t != '\0')
	  action = S_CATCH;
	else
	  action = S_IGN;
	if (rootShell && action == S_DFL) {
		switch (sigNum) {
			case SIGINT:
				if (iflag)
				  action = S_CATCH;
				break;
			case SIGQUIT:
				/* Fall through */
			case SIGTERM:
				if (iflag)
				  action = S_IGN;
				break;
		}
	}
	t = &sigMode[sigNum - 1];
	if (*t == 0) {	/* Current setting unknown */
		/* There is a race condition here if action is not S_IGN.
		 * A signal can be ignored that shouldn't be.
		 */
		if ((int) (sigAct = signal(sigNum, SIG_IGN)) == -1)
		  error("Signal system call failed");
		if (sigAct == SIG_IGN)
		  *t = S_HARD_IGN;
		else
		  *t = S_IGN;
	}
	if (*t == S_HARD_IGN || *t == action)
	  return 0;
	switch (action) {
		case S_DFL:
			sigAct = SIG_DFL;
			break;
		case S_CATCH:
			sigAct = onSig;
			break;
		case S_IGN:
			sigAct = SIG_IGN;
			break;
	}
	*t = action;
	return (int) signal(sigNum, sigAct);	
}

/* Clear traps on a fork.
 */
void clearTraps() {
	char **tp;

	for (tp = trap; tp <= &trap[MAX_SIG]; ++tp) {
		if (*tp && **tp) {	/* trap not NULL or SIG_IGN */
			INTOFF;
			ckFree(*tp);
			*tp = NULL;
			if (tp != &trap[0])
			  setSignal(tp - trap);
			INTON;
		}
	}
}

void exitShell(int status) {
	//TODO
	
	_exit(status);
}

/* Ignore a signal. */
void ignoreSig(int sigNum) {
	if (sigMode[sigNum - 1] != S_IGN && 
			sigMode[sigNum - 1] != S_HARD_IGN) {
		signal(sigNum, SIG_IGN);	
	}
	sigMode[sigNum - 1] = S_HARD_IGN;
}

int trapCmd(int argc, char **argv) {
	printf("=== trapCmd\n");//TODO
	return 0;
}


#ifdef mkinit
INCLUDE "signames.h"
INCLUDE "trap.h"

SHELLPROC {
	char *sm;

	clearTraps();
	for (sm = sigMode; sm < sigMode + MAX_SIG; ++sm) {
		if (*sm == S_IGN)
		  *sm = S_HARD_IGN;
	}
}
#endif


