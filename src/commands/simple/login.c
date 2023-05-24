#define _MINIX_SOURCE
#define _POSIX_C_SOURCE 2

#include <sys/types.h>
#include <ttyent.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <utmp.h>
#include <time.h>
#include <sys/utsname.h>
#include <minix/minlib.h>
#include <minix/paths.h>

static char PATH_UTMP[] = _PATH_UTMP;	/* Current logins */
static char PATH_WTMP[] = _PATH_WTMP;	/* Login/logout history */
static char PATH_LASTLOG[] = _PATH_LASTLOG;	/* Last login history */
static char PATH_MOTD[] = _PATH_MOTD;	/* Message of the day */

#define TTY_GID		4
#define EXTRA_ENV	6

/* Crude indication of a tty being physically secure: */
#define secureTty(dev)	((unsigned) ((dev) - 0x400) < (unsigned) 8)

static int timeout;
static char *hostname;
static char user[32];
static char logName[35];
static char home[128];
static char shell[128];
static char term[128];
static char **env;
extern char **environ;


static void usageErr() {
	usage("login", "[-h hostname] [-b] [-f] [-p] [username]");
}

static void handleTimeout(int dummy) {
	stdErr("\r\nLogin timed out after 60 seconds\r\n");
	timeout = 1;
}

static void wtmp(char *user, int uid) {
/* Make entries in /usr/adm/wtmp and /etc/utmp. */
	struct utmp entry;
	register int fd = -1;
	int lineNum;
	int err = 0;
	char *what;


	/* First, read the current UTMP entry. We need some of its
	 * parameters! (like PID, ID etc...).
	 */
	what = "ttyslot()"; 
	lineNum = ttyslot();
	if (lineNum == 0)	/* ttyslot failed */
	  err = errno;

	if (err == 0 && (fd = open(what = PATH_UTMP, O_RDONLY)) < 0) {
		if (errno == ENOENT) 
		  return;
		err = errno;
	}
	if (err == 0 && lseek(fd, (off_t) lineNum * sizeof(entry), SEEK_SET) < 0)
	  err = errno;
	if (err == 0 && read(fd, (char *) &entry, sizeof(entry)) != sizeof(entry))
	  err = errno;
	if (fd >= 0)
	  close(fd);

	/* Enter new fields. */
	strncpy(entry.ut_user, user, sizeof(entry.ut_user));
	if (hostname)
	  strncpy(entry.ut_host, hostname, sizeof(entry.ut_host));

	if (entry.ut_pid == 0)
	  entry.ut_pid = getpid();

	entry.ut_type = USER_PROCESS;	/* We are past login... */
	time(&entry.ut_time);

	/* Write a WTMP record. */
	if (err == 0) {
		if ((fd = open(what = PATH_WTMP, O_WRONLY | O_APPEND)) < 0) {
			if (errno != ENOENT)
			  err = errno;
		} else {
			if (write(fd, (char *) &entry, sizeof(entry)) < 0)
			  err = errno;
			close(fd);
		}
	}

	/* Rewrite the UTMP entry. */
	if (err == 0 && (fd = open(what = PATH_UTMP, O_WRONLY)) < 0)
	  err = errno;
	if (err == 0 && lseek(fd, (off_t) lineNum * sizeof(entry), SEEK_SET) < 0)
	  err = errno;
	if (err == 0 && write(fd, (char *) &entry, sizeof(entry)) < 0)
	  err = errno;
	if (fd >= 0)
	  close(fd);

	/* Write the LASTLOG entry. */
	if (err == 0 && (fd = open(what = PATH_LASTLOG, O_WRONLY)) < 0) {
		if (errno == ENOENT)
		  return;
		err = errno;
	}
	if (err == 0 && lseek(fd, (off_t) uid * sizeof(entry), SEEK_SET) < 0)
	  err = errno;
	if (err == 0 && write(fd, (char *) &entry, sizeof(entry)) < 0)
	  err = errno;
	if (fd >= 0)
	  close(fd);

	if (err != 0) 
	  fprintf(stderr, "login: %s: %s\n", what, strerror(err));
}

static void add2Env(char **env, char *entry, int replace) {
/* Replace an environment variable with entry or add entry if the environment
 * variable doesn't exit yet.
 */
	char *cp;
	int keyLen;

	cp = strchr(entry, '=');
	keyLen = cp - entry + 1;

	for (; *env; ++env) {
		if (strncmp(*env, entry, keyLen) == 0) {
			if (! replace)	/* Don't replace */
			  return;
			break;
		}
	}
	*env = entry;
}

static void showFile(char *name) {
/* Read a textfile and show it on the desired terminal. */
	register int fd, len;
	char buf[80];

	if ((fd = open(name, O_RDONLY)) > 0) {
		do {
			len = read(fd, buf, 80);
			if (len > 0) 
			  write(STDOUT_FILENO, buf, len);
		} while (len > 0);
		close(fd);
	}
}

int main(int argc, char *argv[]) {
	char name[30];
	char *password, *cryptedPwd;
	char *ttyName, *p;
	int n, ap, checkPwd, bad, secure, i, envSize, doBanner;
	struct passwd *pw;
	char *bp, *argx[8], **ep;	/* pw_shell arguments */
	char argx0[64];				/* argv[0] of the shell */
	char *sh = "/bin/sh";		/* sh/pw_shell field value */
	char *initialName;
	int c, bFlag, fFlag, pFlag;
	char *hArg;
	int authorized, preserveEnv;
	struct ttyent *ttyp;
	struct stat ttyStat;
	struct sigaction sa;
	struct utsname uts;

	/* Don't let QUIT dump core. */
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = exit;
	sigaction(SIGQUIT, &sa, NULL);

	/* Parse options. */
	bFlag = fFlag = pFlag = 0;
	hArg = NULL;
	while ((c = getopt(argc, argv, "?bfh:p")) != -1) {
		switch (c) {
			case 'b': bFlag = 1; break;
			case 'f': fFlag = 1; break;
			case 'h':
				if (hArg)
				  usageErr();
				if (getuid() == 0)
				  hArg = optarg;
				break;
			case 'p': pFlag = 1; break;
			case '?':
				usageErr();
			default:
				fprintf(stderr, "login: getopt failed: '%c'\n", c);
				exit(EXIT_FAILURE);
		}
	}
	if (optind < argc) 
	  initialName = argv[optind++];
	else
	  initialName = NULL;
	if (optind != argc)
	  usageErr();

	authorized = fFlag;
	hostname = hArg;
	preserveEnv = pFlag;
	doBanner = bFlag;

	/* Look up /dev/tty number. */
	ttyName = ttyname(STDIN_FILENO);
	if (ttyName == NULL) {
		printf("Unable to lookup tty name\n");
		exit(EXIT_FAILURE);
	}

	if (doBanner) {
		p = strrchr(ttyName, '/');
		if (! p)
		  p = ttyName;
		else
		  ++p;
		uname(&uts);
		tell("\n");
		tell(uts.sysname);
		tell("/");
		tell(uts.machine);
		tell(" Release ");
		tell(uts.release);
		tell(" Version " );
		tell(uts.version);
		tell(" (");
		tell(p);
		tell(")\n\n");
		tell(uts.nodename);
		tell(" ");
	}

	/* Get login name and passwd. */
	for (;; initialName = NULL) {
		if (initialName)
		  strcpy(name, initialName);
		else {
			do {
				tell("login: ");
				n = read(STDIN_FILENO, name, 30);
				if (n == 0)
				  exit(EXIT_FAILURE);
				if (n < 0) {
					if (errno != EINTR)
					  fprintf(stderr, "login: read failed: %s\n", strerror(errno));
					exit(EXIT_FAILURE);
				}
			} while (n < 2);
			name[n - 1] = 0;
		}

		/* Start timer running. */
		timeout = 0;
		sa.sa_handler = handleTimeout;
		sigaction(SIGALRM, &sa, NULL);
		alarm(60);

		/* Look up login/passwd. */
		pw = getpwnam(name);

		checkPwd = 1;			/* Default is check password */

		/* For now, only console is secure. */
		secure = fstat(STDIN_FILENO, &ttyStat) == 0 && 
					secureTty(ttyStat.st_rdev);

		if (pw) {
			if (authorized && initialName &&
					(pw->pw_uid == getuid() || getuid() == 0)) 
			  checkPwd = 0;		/* Don't ask a password for pre-authorized users. */
			else if (secure && 
					strcmp(crypt("", pw->pw_passwd), pw->pw_passwd) == 0)
			  checkPwd = 0;		/* Empty password */
		}

		if (checkPwd) {
			password = getpass("Password:");

			if (timeout)
			  exit(EXIT_FAILURE);

			bad = 0;
			if (! pw)
			  bad = 1;
			if (! password) {
				password = "";
				bad = 1;
			}
			if (! secure && pw && 
					strcmp(crypt("", pw->pw_passwd), pw->pw_passwd) == 0)
			  bad = 1;

			cryptedPwd = (bad ? "*" : pw->pw_passwd);

			if (strcmp(crypt(password, cryptedPwd), cryptedPwd) != 0) {
				tell("Login incorrect\n");
				continue;
			}
		}

		/* Check if the system is going down. */
		if (access("/etc/nologin", 0) == 0 && strcmp(name, "root") != 0) {
			tell("System going down\n\n");
			continue;
		}

		/* Stop timer. */
		alarm(0);

		/* Write login record to /usr/adm/wtmp and /etc/utmp */
		wtmp(name, pw->pw_uid);

		/* Create the arg[] array from the pw_shell field. */
		ap = 0;
		argx[ap++] = argx0;		/* "-sh" most likely */
		if (pw->pw_shell[0]) {
			sh = pw->pw_shell;
			bp = sh;
			while (*bp) {
				while (*bp && *bp != ' ' && *bp != '\t') {
					++bp;
				}
				if (*bp == ' ' || *bp == '\t') {
					*bp++ = '\0';	/* Mark end of string */
					argx[ap++] = bp;
				}
			}
		} else {
			argx[ap] = NULL;
		}
		strcpy(argx0, "-");	/* Most shells need it for their .profile */
		if ((bp = strrchr(sh, '/')) == NULL)
		  bp = sh;
		else
		  ++bp;
		strncat(argx0, bp, sizeof(argx0) - 2);

		/* Set the environment */
		if (preserveEnv) {
			for (ep = environ; *ep; ++ep) {
			}
		} else {
			ep = environ;
		}
		envSize = ep - environ;
		env = calloc(envSize + EXTRA_ENV, sizeof(*env));
		if (env == NULL) {
			fprintf(stderr, "login: out of memory\n");
			exit(EXIT_FAILURE);
		}
		for (i = 0; i < envSize; ++i) {
			env[i] = environ[i];
		}

		strcpy(user, "USER=");
		strcat(user, name);
		add2Env(env, user, 1);

		strcpy(logName, "LOGNAME=");
		strcat(logName, name);
		add2Env(env, logName, 1);

		strcpy(home, "HOME=");
		strcat(home, pw->pw_dir);
		add2Env(env, home, 1);

		strcpy(shell, "SHELL=");
		strcat(shell, sh);
		add2Env(env, shell, 1);

		if ((ttyp = getttynam(ttyName + 5)) != NULL) {	/* +5 is for "/dev/" */
			strcpy(term, "TERM=");
			strcat(term, ttyp->ty_type);
			add2Env(env, term, 0);
		}

		/* Show the message-of-the-day. */
		showFile(PATH_MOTD);

		/* Assign the terminal to this user. */
		chown(ttyName, pw->pw_uid, TTY_GID);
		chmod(ttyName, 0620);

		/* Change id. */
		setgid(pw->pw_gid);
		setuid(pw->pw_uid);

		/* cd $HOME */
		chdir(pw->pw_dir);

		/* Reset signals to default values. */
		sa.sa_handler = SIG_DFL;
		for (n = 1; n <= NSIG; ++n) {
			sigaction(n, &sa, NULL);
		}

		/* Execute the user's shell. */
		execve(sh, argx, env);

		if (pw->pw_gid == 0) {
			/* Privileged user gets /bin/sh in times of crisis. */
			sh = "/bin/sh";
			argx[0] = "-sh";
			strcpy(shell, "SHELL=");	/* Change env */
			strcat(shell, sh);
			execve(sh, argx, env);
		}
		fprintf(stderr, "login: can't execute %s: %s\n", sh, strerror(errno));
		exit(EXIT_FAILURE);
	}
	return 0;
}


