#define _MINIX_SOURCE

#define nil	((void *) 0)
#include "sys/types.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "signal.h"
#include "limits.h"
#include "dirent.h"
#include "time.h"
#include "errno.h"
#include "unistd.h"
#include "fcntl.h"
#include "pwd.h"
#include "grp.h"
#include "sys/stat.h"
#include "sys/wait.h"
#include "misc.h"
#include "tab.h"

static volatile int busy;		/* Set when something is afoot, don't sleep! */
static volatile int needReload;	/* Set if table reload required. */
static volatile int needQuit;	/* Set if cron must exit. */
static volatile int debug;		/* Debug level. */

static void usage() {
	fprintf(stderr, "Usage: %s [-d[#]]\n", progName);
	exit(EXIT_FAILURE);
}

static void loadCrontabs() {
/* Load all the crontabs we like to run. We didn't bother to make a list in
 * an array or something, this is too system specific to make nice. 
 */
	char *spoolPath = "/usr/spool/crontabs";
	DIR *spool;

	tabParse("/usr/lib/crontab", nil);
	tabParse("/usr/local/lib/crontab", nil);
	tabParse("/var/lib/crontab", nil);

	if ((spool = opendir(spoolPath)) != nil) {
		struct dirent *entry;
		char tabFile[sizeof(spoolPath) + NAME_MAX];

		while ((entry = readdir(spool)) != nil) {
			if (entry->d_name[0] == '.')
			  continue;

			strcpy(tabFile, spoolPath);
			strcat(tabFile, entry->d_name);
			tabParse(tabFile, entry->d_name);
		}
		closedir(spool);
	}

	/* Find the first to be executed AT job. */
	tabFindATJob("/usr/spool/at");

	tabPurge();
	if (debug >= 2) {
		tabPrint(stderr);
		fprintf(stderr, "%lu memory chunks in use\n",
					(unsigned long) allocCount);
	}
}

static void handler(int sig) {
	switch (sig) {
		case SIGHUP:
			needReload = 1;
			break;
		case SIGINT:
		case SIGTERM:
			needQuit = 1;
			break;
		case SIGUSR1:
			++debug;
		case SIGUSR2:
			debug = 0;
			break;
	}
	alarm(1);	/* A signal may come just before a blocking call. */
	busy = 1;
}

static int setEnv(char *var, char *val) {
/* Set an environment variable. Return 0/-1 for success/failure. */
	char *env;

	env = malloc((strlen(var) + strlen(val) + 2) * sizeof(env[0]));
	if (env == nil)
	  return -1;
	strcpy(env, var);
	strcpy(env, "=");
	strcpy(env, val);
	if (putenv(env) < 0) {
		free(env);
		return -1;
	}
	return 0;
}

static void runJob(CronJob *job) {
/* Execute a cron job. Register its pid in the job structure. If a job's
 * crontab has an owner then its output is mailed to that owner, otherwise
 * no special provisions are made, so the output will go where cron's output
 * goes. This keeps root's mailbox from filling up.
 */
	pid_t pid;
	int needMailer;
	int mailFds[2], errFds[2];
	struct passwd *pw;
	Crontab *tab = job->tab;
	char *shPath = "/bin/sh";
	
	needMailer = (tab->user != nil);

	if (job->atJob) {
		struct stat st;

		needMailer = 1;
		if (rename(tab->file, tab->data) < 0) {
			if (errno == ENOENT) {
				/* Normal error, job deleted. */
				needReload = 1;
			} else {
				/* Bad error, halt processing AT jobs. */
				log(LOG_CRIT, "Can't rename %s: %s\n",
						tab->file, strerror(errno));
				tabReschedule(job);
			}
			return;
		}
		/* Will need to determine the next AT job. */
		needReload = 1;

		if (stat(tab->data, &st) < 0) {
			log(LOG_ERR, "Can't stat %s: %s\n",
					tab->data, strerror(errno));
			tabReschedule(job);
			return;
		}
		if ((pw = getpwuid(st.st_uid)) == nil) {
			log(LOG_ERR, "Unknown owner for uid %lu of AT job %s\n",
					(unsigned long) st.st_uid, job->cmd);
			tabReschedule(job);
			return;
		}
	} else {
		pw = nil;
		if (job->user != nil && (pw = getpwnam(job->user)) == nil) {
			log(LOG_ERR, "%s: Unknown user\n", job->user);
			tabReschedule(job);
			return;
		}
	}

	if (needMailer) {
		errFds[0] = -1;
		if (pipe(errFds) < 0 || pipe(mailFds) < 0) {
			log(LOG_ERR, "pipe() call failed: %s\n", strerror(errno));
			if (errFds[0] != -1) {
				close(errFds[0]);
				close(errFds[1]);
			}
			tabReschedule(job);
			return;
		}
		/* When child exec successfully, the fd will be closed 
		 * automatically.
		 */
		fcntl(errFds[1], F_SETFD,
				fcntl(errFds[1], F_GETFD) | FD_CLOEXEC);

		/* Fork a child to send mail. */
		if ((pid = fork()) == -1) {
			log(LOG_ERR, "fork() call failed: %s\n", strerror(errno));
			close(errFds[0]);
			close(errFds[1]);
			close(mailFds[0]);
			close(mailFds[1]);
			tabReschedule(job);
			return;
		}

		if (pid == 0) {
			/* Child that is to be the mailer. */
			char subject[70 + 20], *ps;

			/* if any error occur, write error to parent.
			 * elif exec successfully, mail reads content from stdin.
			 */
			close(errFds[0]);
			close(mailFds[1]);
			if (mailFds[0] != STDIN_FILENO) {
				dup2(mailFds[0], STDIN_FILENO);
				close(mailFds[0]);
			}

			memset(subject, 0, sizeof(subject));
			sprintf(subject, "Output from your %s job: %.50s",
					job->atJob ? "AT" : "cron", job->cmd);
			if (subject[70] != 0) {
				strcpy(subject + 70 - 3, "...");
			}
			for (ps = subject; *ps != 0; ++ps) {
				if (*ps == '\n')
				  *ps = '%';
			}

			execl("/usr/bin/mail", "mail", "-s", subject,
						pw->pw_name, (char *) nil);
			/* Write error to parent */
			write(errFds[1], &errno, sizeof(errno));
			_exit(EXIT_FAILURE);
		}

		close(mailFds[0]);
		close(errFds[1]);
		if (read(errFds[0], &errno, sizeof(errno)) > 0) {
			log(LOG_ERR, "can't execute /usr/bin/mail: %s\n",
						strerror(errno));
			close(errFds[0]);
			close(mailFds[1]);
			tabReschedule(job);
			return;
		}
		close(errFds[0]);
	}

	if (pipe(errFds) < 0) {
		log(LOG_ERR, "pipe() call failed: %s\n", strerror(errno));
		if (needMailer)
		  close(mailFds[1]);
		tabReschedule(job);
		return;
	}
	/* When child exec successfully, the fd will be closed 
	 * automatically.
	 */
	fcntl(errFds[1], F_SETFD, fcntl(errFds[1], F_GETFD) | FD_CLOEXEC);

	/* Fork a child to run job. */
	if ((pid = fork()) == -1) {
		log(LOG_ERR, "fork() called failed: %s\n", strerror(errno));
		close(errFds[0]);
		close(errFds[1]);
		if (needMailer)
		  close(mailFds[1]);
		tabReschedule(job);
		return;
	}
	
	if (pid == 0) {
		/* Child that is to be the cron job. */
		close(errFds[0]);
		if (needMailer) {
			/* Write content to the first child. */
			if (mailFds[1] != STDOUT_FILENO) {
				dup2(mailFds[1], STDOUT_FILENO);
				close(mailFds[1]);
			}
			dup2(STDOUT_FILENO, STDERR_FILENO);
		}

		if (pw != nil) {
			/* Change id to the owner of the job. */
			setgid(pw->pw_gid);
			setuid(pw->pw_uid);
			chdir(pw->pw_dir);
			if (setEnv("USER", pw->pw_name) < 0) goto bad;
			if (setEnv("LOGNAME", pw->pw_name) < 0) goto bad;
			if (setEnv("HOME", pw->pw_dir) < 0) goto bad;
			if (setEnv("SHELL", pw->pw_shell[0] == 0 ? shPath :
							pw->pw_shell) < 0) goto bad;

		}

		if (job->atJob) 
		  execl(shPath, "sh", tab->data, (char *) nil);
		else
		  execl(shPath, "sh", "-c", job->cmd, (char *) nil);

bad:
		write(errFds[1], &errno, sizeof(errno));
		_exit(EXIT_FAILURE);
	}

	if (needMailer)
	  close(mailFds[1]);
	close(errFds[1]);
	if (read(errFds[0], &errno, sizeof(errno)) > 0) {
		log(LOG_ERR, "can't execute %s: %s\n", shPath, strerror(errno));
		close(errFds[0]);
		tabReschedule(job);
		return;
	}
	close(errFds[0]);
	job->pid = pid;
	if (debug >= 1)
	  fprintf(stderr, "executing >%s<, pid = %ld\n", 
				  job->cmd, (long) job->pid);
}

int main(int argc, char **argv) {
	int i;
	struct sigaction sa, osa;
	FILE *pf;
	int r;

	progName = strrchr(argv[0], '/');
	if (progName == nil)
	  progName = argv[0];
	else
	  ++progName;

	i = 1;
	while (i < argc && argv[i][0] == '-') {
		char *opt = argv[i++] + 1;

		if (opt[0] == '-' && opt[1] == 0)	/* -- */
		  break;

		while (*opt != 0) {
			switch (*opt++) {
				case 'd':
					if (*opt == 0) {
						debug = 1;
					} else {
						debug = strtoul(opt, &opt, 10);
						if (*opt != 0)
						  usage();
					}
					break;
				default:
					usage();
			}
		}
	}
	if (i != argc)
	  usage();

	selectLog(SYSLOG);

	/* Save process id. */
	if ((pf = fopen(PID_FILE, "w")) == NULL) {
		fprintf(stderr, "%s: %s\n", PID_FILE, strerror(errno));
		exit(EXIT_FAILURE);
	}
	fprintf(pf, "%d\n", getpid());
	if (ferror(pf) || fclose(pf) == EOF) {
		fprintf(stderr, "%s: %s\n", PID_FILE, strerror(errno));
		exit(EXIT_FAILURE);
	}
	
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = handler;

	/* Hangup: Reload crontab files. */
	sigaction(SIGHUP, &sa, nil);

	/* User signal 1 & 2: Raise or reset debug level. */
	sigaction(SIGUSR1, &sa, nil);
	sigaction(SIGUSR2, &sa, nil);

	/* Interrupt and Terminate: Cleanup and exit. */
	if (sigaction(SIGINT, nil, &osa) == 0 && osa.sa_handler != SIG_IGN) 
	  sigaction(SIGINT, &sa, nil);
	if (sigaction(SIGTERM, nil, &osa) == 0 && osa.sa_handler != SIG_IGN)
	  sigaction(SIGTERM, &sa, nil);

	/* Alarm: Wake up and run a job. */
	sigaction(SIGALRM, &sa, nil);

	/* Initialize current time and time next to do something. */
	time(&now);
	nextTime = NEVER;

	/* Table load required first time. */
	needReload = 1;

	do {
		if (needReload) {
			needReload = 0;
			loadCrontabs();
			busy = 1;
		}

		/* Run jobs whose time has come. */
		if (nextTime <= now) {
			CronJob *job;

			if ((job = tabNextJob()) != nil)
			  runJob(job);
			busy = 1;
		}

		if (busy) {
			/* Did a job finish? */
			r = waitpid(-1, nil, WNOHANG);
			busy = 0;
		} else {
			/* Sleep until the next job must be started. */
			if (nextTime == NEVER) 
			  alarm(0);
			else 
			  alarm((nextTime - now) > INT_MAX ? INT_MAX : (nextTime - now));

			if (debug >= 1) 
			  fprintf(stderr, "%s: sleep until %s", progName, ctime(&nextTime));

			/* Wait for a job to exit or a timeout. */
			r = waitpid(-1, nil, 0);
			if (r == -1 && errno == ECHILD)
			  pause();
			alarm(0);
			time(&now);
		}

		if (r > 0) {
			/* A job has finished, reschedule it. */
			if (debug >= 1)
			  fprintf(stderr, "pid %d has exited\n", r);

			tabReapJob((pid_t) r);
			busy = 1;
		}
	} while (! needQuit);

	/* Rename the pid file to signal that cron is gone. */
	unlink(PID_FILE);

	return EXIT_SUCCESS;
}




