#include <minix/type.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/svrctl.h>
#include <ttyent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <utmp.h>
#include <minix/minlib.h>

/* Command to execute as a response to the three finger salute. */
char *REBOOT_CMD[] = { "shutdown", "now", "CTRL-ALT-DEL", NULL };

/* Associated fake ttytab entry. */
static struct ttyent TT_REBOOT = { "console", "-", REBOOT_CMD, NULL };

static char PATH_UTMP[] = "/etc/utmp";		/* Current logins */
static char PATH_WTMP[] = "/usr/adm/wtmp";	/* Login/logout history */
static char *prog = "init";

#define PID_SLOTS	32		/* First this many ttys can be on */

typedef struct {
	int errCount;			/* Error count */
	pid_t pid;				/* Pid of login process for this tty line */
} SlotEntry;

#define ERR_COUNT_DISABLE	10	/* Disable after this many errors */
#define NO_PID	0			/* Pid value indicating no process */

static SlotEntry slots[PID_SLOTS];	/* Init table of ttys and pids */

static int gotHup = 0;		/* Flag, showing signal 1 was received */
static int gotAbort = 0;	/* Flag, showing signal 6 was received */
static int spawn = 1;		/* Flag, spawn processes only when set */

static int execute(char **cmd) {
/* Execute a command with a path search along /sbin:/bin:/usr/sbin:/usr/bin. */
	static char *nullEnv[] = { NULL };
	char command[128];
	char *path[] = { "/sbin", "/bin", "/usr/sbin", "/usr/bin" };
	int i;

	if (cmd[0][0] == '/') {
		/* A full path. */
		return execve(cmd[0], cmd, nullEnv);
	}

	/* Path search. */
	for (i = 0; i < 4; ++i) {	
		if (strlen(path[i]) + 1 + strlen(cmd[0]) + 1 > sizeof(command)) {
			errno = ENAMETOOLONG;
			return -1;
		}
		strcpy(command, path[i]);
		strcat(command, "/");
		strcat(command, cmd[0]);
		execve(command, cmd, nullEnv);
		if (errno != ENOENT)
		  break;
	}
	return -1;
}

static void onHup(int sig) {
	gotHup = 1;
	spawn = 1;
}

static void onTerm(int sig) {
	spawn = 0;
}

static void onAbrt(int sig) {
	static int count;

	if (++count == 2)
	  reboot(RBT_HALT);
	gotAbort = 1;
}

static void wtmp(
	int type,		/* Type of entry */
	int lineNum,	/* Line number in ttytab */
	char *line,		/* Tty name (only good on login) */
	pid_t pid		/* Pid of process */
) {
/* Log an event into the UTMP and WTMP files. */
	
	struct utmp utmp;	/* UTMP/WTMP User Accounting */
	int fd;

	/* Clear the utmp record. */
	memset((void *) &utmp, 0, sizeof(utmp));

	/* Fill in utmp. */
	switch (type) {
		case BOOT_TIME:
			/* Make a special reboot record. */
			strcpy(utmp.ut_name, "reboot");
			strcpy(utmp.ut_line, "~");
			break;

		case LOGIN_PROCESS:
			/* A new login, fill in line name. */
			strncpy(utmp.ut_line, line, sizeof(utmp.ut_line));
			break;

		case DEAD_PROCESS:
			/* A logout. Use the current utmp entry, but make sure it is a
			 * user process exiting, and not getty or login giving up.
			 */
			if ((fd = open(PATH_UTMP, O_RDONLY)) < 0) {
				if (errno != ENOENT)
				  reportStdErr(prog, PATH_UTMP);
				return;
			}
			if (lseek(fd, (off_t) (lineNum + 1) * sizeof(utmp), SEEK_SET) == -1 ||
						read(fd, &utmp, sizeof(utmp)) == -1) {
				reportStdErr(prog, PATH_UTMP);
				close(fd);
				return;
			}
			close(fd);
			if (utmp.ut_type != USER_PROCESS)
			  return;
			strncpy(utmp.ut_name, "", sizeof(utmp.ut_name));
			break;
	}

	/* Finish new utmp entry. */
	utmp.ut_pid = pid;
	utmp.ut_type = type;
	utmp.ut_time = time((time_t *) 0);

	switch (type) {
		case LOGIN_PROCESS:
		case DEAD_PROCESS:
			/* Write new entry to utmp. */
			if ((fd = open(PATH_UTMP, O_WRONLY)) < 0 ||
					lseek(fd, (off_t) (lineNum + 1) * sizeof(utmp), SEEK_SET) == -1 ||
					write(fd, &utmp, sizeof(utmp)) == -1) {
				if (errno != ENOENT)
				  reportStdErr(prog, PATH_UTMP);
			}
			if (fd != -1)
			  close(fd);
			break;
	}

	switch (type) {
		case BOOT_TIME:
		case DEAD_PROCESS:
			/* Add new wtmp entry. */
			if ((fd = open(PATH_WTMP, O_WRONLY | O_APPEND)) < 0 ||
					write(fd, &utmp, sizeof(utmp)) == -1) {
				if (errno != ENOENT)
				  reportStdErr(prog, PATH_WTMP);
			}
			if (fd != -1)
			  close(fd);
			break;
	}
}

static void startup(int lineNum, struct ttyent *ttyp) {
/* Fork off a process for the indicated line. */
	
	SlotEntry *slotp;	/* Pointer to ttyslot */
	pid_t pid;			/* New pid */
	int err[2];			/* Error reporting pipe */
	char line[32];		/* Tty device name */
	int status;

	slotp = &slots[lineNum];

	/* Error channel for between fork and exec. */
	if (pipe(err) < 0)
	  err[0] = err[1] = -1;

	if ((pid = fork()) == -1) {
		reportStdErr(prog, "fork()");
		sleep(10);
		return;
	}

	if (pid == 0) {	/* Child(startup) */
		close(err[0]);
		fcntl(err[1], F_SETFD, fcntl(err[1], F_GETFD) | FD_CLOEXEC);

		/* Make child to be a session leader, and map the line to 
		 * /dev/tty as the ctty.
		 * 
		 *	line0 fp_tty = /dev/console
		 *	line1 fp_tty = /dev/ttyc1
		 *	line2 fp_tty = /dev/ttyc2
		 *	line3 fp_tty = /dev/ttyc3
		 *
		 * When read/write /dev/tty, since it is a ctty, it calls cttyIO() 
		 * which uses the line's fp_tty as the actual tty.
		 */

		/* A new session */
		setsid();
		
		/* Construct device name. */
		strcpy(line, "/dev/");
		strncat(line, ttyp->ty_name, sizeof(line) - 6);

		/* Open the line for standard input and output. */
		close(STDIN_FILENO);
		close(STDOUT_FILENO);
		if (open(line, O_RDWR) < 0 || dup(STDIN_FILENO) < 0) {
			write(err[1], &errno, sizeof(errno));
			_exit(EXIT_FAILURE);
		}

		if (ttyp->ty_init != NULL && ttyp->ty_init[0] != NULL) {
			/* Execute a command to initialize the terminal line. */

			if ((pid = fork()) == -1) {
				reportStdErr(prog, "fork()");
				errno = 0;
				write(err[1], &errno, sizeof(errno));
				_exit(EXIT_FAILURE);
			}

			if (pid == 0) {	/* Child(ttyInit) */
				alarm(10);
				execute(ttyp->ty_init);
				reportStdErr(prog, ttyp->ty_init[0]);
				_exit(EXIT_FAILURE);
			}

			/* Parent(startup) */
			while (waitpid(pid, &status, 0) != pid) {
			}

			if (status != 0) {	/* ttyInit failed */
				stdErr(prog);
				stdErr(": ");
				stdErr(ttyp->ty_name);
				stdErr(": ");
				stdErr(ttyp->ty_init[0]);
				stdErr(": bad exit status\n");
				errno = 0;
				write(err[1], &errno, sizeof(errno));
				_exit(EXIT_FAILURE);
			}
		}

		/* Redirect standard error too. */
		dup2(STDIN_FILENO, STDERR_FILENO);

		/* Execute the getty process. */
		execute(ttyp->ty_getty);

		/* Oops, disater strikes. */
		fcntl(STDERR_FILENO, F_SETFL, 
				fcntl(STDERR_FILENO, F_GETFL) | O_NONBLOCK);
		if (lineNum != 0)
		  reportStdErr(prog, ttyp->ty_getty[0]);
		write(err[1], &errno, sizeof(errno));
		_exit(EXIT_FAILURE);
	}

	/* Parent(init) */
	if (ttyp != &TT_REBOOT)
	  slotp->pid = pid;

	close(err[1]);
	if (read(err[0], &errno, sizeof(errno)) != 0) {	/* startup failed */
		/* If an errno value goes down the error pipe: Problems. */
		
		switch (errno) {
			case ENOENT:
			case ENODEV:
			case ENXIO:
				/* Device nonexistent, no driver, or no minor device. */
				slotp->errCount = ERR_COUNT_DISABLE;
				close(err[0]);
				return;
			case 0:
				/* Error already reported. */
				break;
			default:
				/* Any other error on the line. */
				reportStdErr(prog, ttyp->ty_name);
		}
		close(err[0]);

		if (++slotp->errCount >= ERR_COUNT_DISABLE) {
			stdErr(prog);
			stdErr(": ");
			stdErr(ttyp->ty_name);
			stdErr(": excessive errors, shutting down\n");
		} else {
			sleep(5);
		}
		return;
	}

	/* startup success */
	close(err[0]);

	if (ttyp != &TT_REBOOT)
	  wtmp(LOGIN_PROCESS, lineNum, ttyp->ty_name, pid);
	slotp->errCount = 0;
}

void main() {
	pid_t pid;		/* Pid of child process */
	int fd;
	int lineNum;	
	int check;		/* Check if a new process must be spawned */
	SlotEntry *slotp;	
	struct ttyent *ttyp;
	struct stat st;
	struct sigaction sa;

	if (fstat(0, &st) < 0) {
		/* Open standard input, output & error. */
		open("/dev/null", O_RDONLY);
		open("/dev/log", O_WRONLY);
		dup(STDOUT_FILENO);		/* 2 > &1 */
	}

	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	/* Hangup: Reexamine /etc/ttytab for newly enabled terminal lines. */
	sa.sa_handler = onHup;
	sigaction(SIGHUP, &sa, NULL);

	/* Terminate: Stop spawning login processes, shutdown is near. */
	sa.sa_handler = onTerm;
	sigaction(SIGTERM, &sa, NULL);

	/* Abort: Sent by the kernel on CTRL-ALT-DEL; shut the system down. */
	sa.sa_handler = onAbrt;
	sigaction(SIGABRT, &sa, NULL);

	/* Execute the /etc/rc file. */
	if ((pid = fork()) != 0) {
		/* Parent(init) just waits for shell init. */
		while (wait(NULL) != pid) {
			if (gotAbort) 
			  reboot(RBT_HALT);
		}
	} else {	
		/* Child(sh) init shell. */
		SysGetEnv sysGetEnv;
		char bootOpts[16];
		static char *rcCommand[] = { "sh", "/etc/rc", NULL, NULL, NULL }; 
		char **rcp = rcCommand + 2;
	
		/* Get the boot options from the boot environment. */
		sysGetEnv.key = "bootopts";
		sysGetEnv.keyLen = 8 + 1;
		sysGetEnv.val = bootOpts;
		sysGetEnv.valLen = sizeof(bootOpts);
		if (svrctl(MM_GET_PARAM, &sysGetEnv) == 0)
		  *rcp++ = bootOpts;
		*rcp = "start";

		execute(rcCommand);
		reportStdErr("sh /etc/rc", NULL);
		_exit(EXIT_FAILURE);	/* Impossible, we hope */
	}

	/* Clear /etc/utmp if it exists. */
	if ((fd = open(PATH_UTMP, O_WRONLY | O_TRUNC)) >= 0)
	  close(fd);

	/* Log system reboot. */
	wtmp(BOOT_TIME, 0, NULL, 0);

	/* Main loop. If login processes have already been started up, wait for one
	 * to terminate, or for a HUP signal to arrive. Start up new login processes
	 * for all ttys which don't have them. Note that wait(0 also returns when
	 * somebody's orphan dies, in which ase ignore it. If the TERM signal is
	 * sent then stop spawning processes, shutdown time is near.
	 */

	check = 1;
	while (1) {
		while ((pid = waitpid(-1, NULL, check ? WNOHANG : 0)) > 0) {
			/* Search to see which line terminated. */
			for (lineNum = 0; lineNum < PID_SLOTS; ++lineNum) {
				slotp = &slots[lineNum];
				if (slotp->pid == pid) {
					/* Record process exiting. */
					wtmp(DEAD_PROCESS, lineNum, NULL, pid);
					slotp->pid = NO_PID;
					check = 1;
				}
			}
		}

		/* If a signal 1 (SIGHUP) is received, simply reset error counts. */
		if (gotHup) {
			gotHup = 0;
			for (lineNum = 0; lineNum < PID_SLOTS; ++lineNum) {
				slots[lineNum].errCount = 0;
			}
			check = 1;
		}

		/* Shut down on signal 6 (SIGABRT). */
		if (gotAbort) {
			gotAbort = 0;
			startup(0, &TT_REBOOT);
		}

		if (spawn && check) {
			/* See which lines need a login process started up. */
			for (lineNum = 0; lineNum < PID_SLOTS; ++lineNum) {
				slotp = &slots[lineNum];
				if ((ttyp = getttyent()) == NULL)
				  break;

				if (ttyp->ty_getty != NULL &&
						ttyp->ty_getty[0] != NULL &&
						slotp->pid == NO_PID &&
						slotp->errCount < ERR_COUNT_DISABLE) 
				  startup(lineNum, ttyp);
			}
			endttyent();
		}
		check = 0;
	}
}

