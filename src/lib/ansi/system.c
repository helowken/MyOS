#if defined(_POSIX_SOURCE)
#include <sys/types.h>
#endif
#include <stdlib.h>
#include <sys/wait.h>

extern pid_t fork(void);
extern pid_t wait(int *);
extern void _exit(int);
extern void execve(const char *path, const char **argv, const char **envp);
extern int close(int);

#define FAIL	127

extern const char ***_penviron;
static const char *execTab[] = {
	"sh",	/* argv[0] */
	"-c",	/* argument to the shell */
	NULL,	/* to be filled with user command */
	NULL	/* terminating NULL */
};

int system(const char *command) {
/* See manual page */

	int pid, exitStatus, waitVal;
	int i;

	if ((pid = fork()) < 0)
	  return command ? -1 : 0;

	if (pid == 0) {		/* Child */
		for (i = 3; i <= 20; ++i) {
			close(i);
		}
		if (! command)
		  command = "cd .";		/* Just testing for a shell */

		execTab[2] = command;	/* Fill in command */
		execve("/bin/sh", execTab, *_penviron);

		/* A shell could not be executed in the child process. */
		_exit(FAIL);		
	}

	/* Parent */
	while ((waitVal = wait(&exitStatus)) != pid) {
		if (waitVal == -1)
		  break;
	}
	if (waitVal == -1) {	/* Could not retrieve the child status */
		/* No child or maybe interrupted */
		exitStatus = -1;
	}
	if (! command) {	/* Command is NULL */
		if (WEXITSTATUS(exitStatus) == FAIL) /* No shell is available */
		  exitStatus = 0;
		else  /* A shell is available */
		  exitStatus = 1;
	}

	/* All system calls succeed, return the termination status of the
	 * child shell used to execute command.
	 */
	return exitStatus;
}
