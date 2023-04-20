#define nil	0
#include "sys/types.h"
#include "stdlib.h"
#include "string.h"
#include "fcntl.h"
#include "unistd.h"
#include "minix/minlib.h"

static char *msg = "reboot call failed\n";

int main(int argc, char **argv) {
	char *prog;

	execv("/usr/bin/halt", argv);

	prog = getProg(argv);

	sleep(2);	/* Not too fast. */

	reboot(strcmp(prog, "reboot") == 0 ? RBT_REBOOT : RBT_HALT);

	write(STDERR_FILENO, msg, strlen(msg));
	return 1;
}
