#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <time.h>
#include <minix/minlib.h>

int main(int argc, char **argv) {
	register char *name, *password;
	char *shell, sh0[100];
	char fromUser[8 + 1], fromShell[100];
	register struct passwd *pw;
	char USER[20], LOGNAME[25], HOME[100], SHELL[100];
	char *envv[20], **envp;
	int smallEnv;
	char *p;
	int super;
	int loginShell;

	smallEnv = 0;
	loginShell = 0;
	if (argc > 1 && (strcmp(argv[1], "-") == 0 || strcmp(argv[1], "-e") == 0)) {
		if (argv[1][1] == 0)
		  loginShell = 1;	/* 'su -' reads .profile */
		argv[1] = argv[0];
		++argv;
		--argc;
		smallEnv = 1;	/* Use small environment. */
	}
	if (argc > 1) {
		if (argv[1][0] == '-') {
			fprintf(stderr,
				"Usage: su [-[e]] [user [shell-arguments ...]]\n");
			exit(1);
		}
		name = argv[1];
		argv[1] = argv[0];
		++argv;
	} else {
		name = "root";
	}

	if ((pw = getpwuid(getuid())) == NULL) {
		fprintf(stderr, "You do not exist\n");
		exit(1);
	}
	strncpy(fromUser, pw->pw_name, 8);
	fromUser[8] = 0;
	strncpy(fromShell, pw->pw_shell[0] == 0 ? "/bin/sh" : pw->pw_shell,
				sizeof(fromShell) - 1);
	fromShell[sizeof(fromShell) - 1] = 0;
	
	if ((pw = getpwnam(name)) == NULL) {
		fprintf(stderr, "Unknown id: %s\n", name);
		exit(1);
	}
	super = 0;
	if (getgid() == 0)
	  super = 1;

	if (! super && strcmp(pw->pw_passwd, crypt("", pw->pw_passwd)) != 0) {
		password = getpass("Password:");
		if (password == NULL ||
				strcmp(pw->pw_passwd, crypt(password, pw->pw_passwd)) != 0) {
			if (password != NULL && *password != 0) {
				fprintf(stderr, "Sorry\n");
				exit(2);
			}
		}
	}

	setgid(pw->pw_gid);
	setuid(pw->pw_uid);
	if (loginShell) {
		shell = pw->pw_shell[0] == 0 ? "/bin/sh" : pw->pw_shell;
	} else {
		if ((shell = getenv("SHELL")) == NULL)
		  shell = fromShell;
	}
	if ((p = strrchr(shell, '/')) == NULL)
	  p = shell; 
	else
	  ++p;
	sh0[0] = '-';
	strcpy(loginShell ? sh0 + 1 : sh0, p);
	argv[0] = sh0;

	if (smallEnv) {
		envp = envv;
		*envp++ = "PATH=:/bin:/usr/bin";

		strcpy(USER, "USER=");
		strcpy(USER + 5, name);
		*envp++ = USER;

		strcpy(LOGNAME, "LOGNAME=");
		strcpy(LOGNAME + 8, name);
		*envp++ = LOGNAME;

		strcpy(SHELL, "SHELL=");
		strcpy(SHELL + 6, shell);
		*envp++ = SHELL;

		strcpy(HOME, "HOME=");
		strcpy(HOME + 5, pw->pw_dir);
		*envp++ = HOME;

		if ((p = getenv("TERM")) != NULL) 
		  *envp++ = p - 5;	/* -5 -> "TERM=" */
		if ((p = getenv("TERMCAP")) != NULL)
		  *envp++ = p - 8;	/* -8 -> "TERMCAP=" */
		if ((p = getenv("TZ")) != NULL)
		  *envp++ = p - 3;	/* -3 -> "TZ=" */

		*envp = NULL;

		chdir(pw->pw_dir);
		execve(shell, argv, envv);
	} else {
		execv(shell, argv);
	}
	fprintf(stderr, "No shell\n");
	return 3;
}

