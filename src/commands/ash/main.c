#include "sys/types.h"
#include "unistd.h"
#include "signal.h"
#include "fcntl.h"
#include "shell.h"
#include "main.h"
#include "eval.h"
#include "trap.h"

int rootPid;
int rootShell;

void main(int arc, char **argv) {
/* Main routine. We initialize things, parse the arguments, execute
 * profiles if we're a login shell, and then call cmdloop to execute
 * commands. The setjmp call sets up the location to jump to when an
 * exception occurs. When an exception occurs the variable "state"
 * is used to figure out how far we had gotten.
 */

	//volatile int state;

	//state = 0;
	
	//TODO if (setjmp(xxx)) {
	//}
	//handler = &jmploc;
	
	rootPid = getpid();
	rootShell = 1;

	//TODO
	exitStatus = 0;
	//TODO
	
	exitShell(exitStatus);
}
