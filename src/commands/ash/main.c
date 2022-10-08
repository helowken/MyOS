#include "sys/types.h"
#include "signal.h"
#include "fcntl.h"
#include "unistd.h"
#include "shell.h"
#include "main.h"
#include "mail.h"
#include "options.h"
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
//TODO
}

void main(int argc, char **argv) {
/* Main routine. We initialize things, parse the arguments, execute
 * profiles if we're a login shell, and then call cmdloop to execute
 * commands. The setjmp call sets up the location to jump to when an
 * exception occurs. When an exception occurs the variable "state"
 * is used to figure out how far we had gotten.
 */
	StackMark stackMark;
	//volatile int state;

	//state = 0;
	
	//TODO if (setjmp(xxx)) {
	//}
	//handler = &jmploc;
	
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
		//state = 1;
		readProfile("/etc/profile");
//state1:
		//state = 2;
	}

	//TODO
	exitStatus = 0;
	//TODO


	//===TODO start
	printf("===sh, argc: %d\n", argc);
	for (int i = 0; i < argc; ++i) {
		printf("%d: %s\n", i, argv[i]);
	}
	//===TODO end

	exitShell(exitStatus);
}
