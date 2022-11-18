


#include "stdlib.h"
#include "signal.h"
#include "fcntl.h"
#include "unistd.h"
#include "shell.h"
#include "main.h"
#include "mail.h"
#include "options.h"
#include "output.h"
#include "parser.h"
#include "nodes.h"
#include "eval.h"
#include "input.h"
#include "trap.h"
#include "stdio.h"
#if ATTY
#include "var.h"
#endif
#include "memalloc.h"
#include "error.h"
#include "init.h"
#include "mystring.h"

int rootPid;
int rootShell;
Node *currCmd;
Node *prevCmd;

/* Read /etc/profile or .profile. Return an error.
 */
static void readProfile(char *name) {
	int fd;

	INTOFF;
	if ((fd = open(name, O_RDONLY)) >= 0)
	  setInputFd(fd, 1);
	INTON;
	if (fd < 0)
	  return;
	cmdLoop(0);
	popFile();
}

/* Read and execute commands. "Top" is nonzero for the top level command
 * loop; it turns on prompting if the shell is interactive.
 */
void cmdLoop(int top) {
	Node *n;
	StackMark stackMark;
	int inter;
	int numEOF;

	setStackMark(&stackMark);
	numEOF = 0;
	//for (;;) {
	for (int i = 0; i < 3; ++i) {//TODO
		if (pendingSigs)
		  doTrap();
		inter = 0;
		if (iFlag && top) {
			++inter;
			//NOT SUPPORT showJobs(1);
			checkMail(0);
			flushOut(&output);
		}
		n = parseCmd(inter);
		if (n != NULL && n != NEOF)
			printf("cmd: %d\n", n->type);//TODO
		if (n == NEOF) {
			if (IFlag == 0 || numEOF >= 50)
				break;
			out2Str("\nUse \"exit\" to leave shell.\n");
			++numEOF;
		} else if (n != NULL && nFlag == 0) {
			printf("=== 222 evalTree\n");
			if (inter) {
				INTOFF;
				if (prevCmd)
				  freeFunc(prevCmd);
				prevCmd = currCmd;
				currCmd = copyFunc(n);
				INTON;
			}
			evalTree(n, 0);
		}
		popStackMark(&stackMark);
	}
	popStackMark(&stackMark);
}

void main(int argc, char **argv) {
/* Main routine. We initialize things, parse the arguments, execute
 * profiles if we're a login shell, and then call cmdloop to execute
 * commands. The setjmp call sets up the location to jump to when an
 * exception occurs. When an exception occurs the variable "state"
 * is used to figure out how far we had gotten.
 */
	JmpLoc jmpLoc;
	StackMark stackMark;
	volatile int state;
	char *shInit, *home;
	char *profile = NULL, *ashrc = NULL;


	//===TODO start
	printf("===sh, argc: %d\n", argc);
	for (int i = 0; i < argc; ++i) {
		printf("%d: %s\n", i, argv[i]);
	}
	//===TODO end

	state = 0;
	if (setjmp(jmpLoc.loc)) {
		/* When a shell procedure is executed, we raise the
		 * exception EX_SHELL_PROC to clean up before executing
		 * the shell procedure.
		 */
		if (exception == EX_SHELL_PROC) {
			rootPid = getpid();
			rootShell = 1;
			minusC = NULL;
			state = 3;
		} else if (state == 0 || iFlag == 0 || ! rootShell) {
		  exitShell(2);
		}
		reset();

		if (exception == EX_INT) {
			out2Char('\n');
			flushOut(&errOut);
		}
		popStackMark(&stackMark);
		FORCE_INTON;		/* Enable interrupts */
		if (state == 1)
		  goto state1;
		else if (state == 2)
		  goto state2;
		else if (state == 3)
		  goto state3;
		else
		  goto state4;
	}
	handler = &jmpLoc;

	rootPid = getpid();
	rootShell = 1;
	init();
	setStackMark(&stackMark);
	procArgs(argc, argv);
	if (eFlag)
	  eFlag = 2;	/* Truely enable [ex]Flag after init. */
	if (xFlag)
	  xFlag = 2;
	if (argv[0] && argv[0][0] == '-') {
		state = 1;
		readProfile("/etc/profile");
state1:
		state = 2;
		if ((home = getenv("HOME")) != NULL &&
				(profile = (char *) malloc(strlen(home) + 10)) != NULL) {
			strcpy(profile, home);
			strcat(profile, "/.profile");
			readProfile(profile);
		} else {
			readProfile(".profile");
		}
	} else if ((sFlag || minusC) && (shInit = getenv("SHINIT")) != NULL) {
		state = 2;
		evalString(shInit);
	}

state2:
	if (profile != NULL)
	  free(profile);

	state = 3;
	if (! argv[0] || argv[0][0] != '-') {
		if ((home = getenv("HOME")) != NULL &&
				(ashrc = (char *) malloc(strlen(home) + 8)) != NULL) {
			strcpy(ashrc, home);
			strcat(ashrc, "/.ashrc");
			readProfile(ashrc);
		}
	}
state3:
	if (ashrc != NULL)
	  free(ashrc);
	if (eFlag)
	  eFlag = 1;	/* Init done, enable [ex]Flag */
	if (xFlag)
	  xFlag = 1;
	exitStatus = 0;	/* Init shouldn't influence initial $? */

	state = 4;
	if (minusC) 
	  evalString(minusC);

	if (sFlag || minusC == NULL) {
state4:
		cmdLoop(1);
	}
	if (true)
	  return;	//TODO

	exitShell(exitStatus);
}




