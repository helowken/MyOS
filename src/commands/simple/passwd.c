#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <pwd.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <minix/minlib.h>
#include <stdio.h>

static char passwdFile[] = "/etc/passwd";
static char shadowFile[] = "/etc/shadow";
static char pwTmpFile[] = "/etc/ptmp";
static char bad[]  = "Permission denied\n";
static char buf[1024];
static char *prog;

enum Action {
	PASSWD, CHFN, CHSH
} action = PASSWD;

static void usageErr() {
	static char *usages[] = {
		"passwd [user]\n",
		"chfn [user] fullname\n",
		"chsh [user] shell\n"
	};
	stdErr(usages[(int) action]);
	exit(1);
}

static int goodChars(char *s) {
	int c;

	while ((c = *s++) != 0) {
		if (c == ':' || c < ' ' || c >= 127)
		  return 0;
	}
	return 1;
}

static void quit(int exStat) {
	if (unlink(pwTmpFile) < 0 && errno != ENOENT) {
		reportStdErr(prog, pwTmpFile);
		exStat = 1;
	}
	exit(exStat);
}

static void fatalErr(char *label) {
	reportStdErr(prog, label);
	quit(1);
}

void main(int argc, char **argv) {
	int uid, cn, n;
	int fdPwd, fdTmp;
	FILE *fpTmp;
	time_t salt;
	struct passwd *pw;
	char *name, pwName[9], oldPwd[9], newPwd[9], newCrypted[14], sl[2];
	char *argN;
	int shadow = 0;

	prog = getProg(argv);
	if (strcmp(prog, "chfn") == 0)
	  action = CHFN;
	else if (strcmp(prog, "chsh") == 0)
	  action = CHSH;

	uid = getuid();

	n = (action == PASSWD ? 1 : 2);
	if (argc != n && argc != n + 1) 
	  usageErr();

	if (argc == n) {	/* For self */
		pw = getpwuid(uid);
		strcpy(pwName, pw->pw_name);
		name = pwName;
	} else {	/* For other user */
		name = argv[1];	
		pw = getpwnam(name);
	}
	if (pw == NULL || ((uid != pw->pw_uid) && uid != 0)) {
		stdErr(bad);
		exit(1);
	}

	switch (action) {
		case PASSWD: {
			if (pw->pw_passwd[0] == '#' && pw->pw_passwd[1] == '#') {
				/* The password is found in the shadow password file. */
				shadow = 1;
				strncpy(pwName, pw->pw_passwd + 2, 8);
				pwName[8] = 0;
				name = pwName;
				setpwfile(shadowFile);
				printf("name: %s\n", name);
				if ((pw = getpwnam(name)) == NULL) {
					stdErr(bad);
					exit(1);
				}
				printf("Changing the shadow password of %s\n", name);
			} else {
				printf("Changing the password of %s\n", name);
			}

			oldPwd[0] = 0;
			if (pw->pw_passwd[0] != 0 && uid != 0) {
				strcpy(oldPwd, getpass("Old password:"));
				if (strcmp(pw->pw_passwd, crypt(oldPwd, pw->pw_passwd)) != 0) {
					stdErr(bad);
					exit(1);
				}
			}
			for (;;) {
				strcpy(newPwd, getpass("New password:"));
				if (newPwd[0] == 0)
				  stdErr("Password cannot be null");
				else if (strcmp(newPwd, getpass("Retype password:")) != 0)
				  stdErr("Passwords don't match");
				else 
				  break;

				stdErr(", try again\n");
			}
			time(&salt);
			sl[0] = (salt & 077) + '.';
			sl[1] = ((salt >> 6) & 077) + '.';
			for (cn = 0; cn < 2; ++cn) {
				if (sl[cn] > '9') 
				  sl[cn] += 7;
				if (sl[cn] > 'Z')
				  sl[cn] += 6;
			}
			strcpy(newCrypted, crypt(newPwd, sl));
			break;
		}
		case CHFN:
		case CHSH:
			argN = argv[argc - 1];
			if (strlen(argN) > (action == CHFN ? 80 : 60) || ! goodChars(argN)) {
				stdErr(bad);
				exit(1);
			}
			break;
	}

	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);

	umask(0);
	n = 10;
	while ((fdTmp = open(pwTmpFile, O_RDWR | O_CREAT | O_EXCL, 0400)) < 0) {
		if (errno != EEXIST)
		  fatalErr("Can't create temporary file");
		if (n-- > 0) {
			sleep(2);
		} else {
			fprintf(stderr, "Password file busy, try again later.\n");
			exit(1);
		}
	}

	if ((fpTmp = fdopen(fdTmp, "w+")) == NULL)
	  fatalErr(pwTmpFile);

	setpwent();
	while ((pw = getpwent()) != 0) {
		if (strcmp(name, pw->pw_name) == 0) {
			switch (action) {
				case PASSWD:
					pw->pw_passwd = newCrypted;
					break;
				case CHFN:
					pw->pw_gecos = argN;
					break;
				case CHSH:
					pw->pw_shell = argN;
					break;
			}
		}
		if (strcmp(pw->pw_shell, "/bin/sh") == 0 ||
				strcmp(pw->pw_shell, "/usr/bin/sh") == 0)
		  pw->pw_shell = "";

		fprintf(fpTmp, "%s:%s:%s:",
				pw->pw_name,
				pw->pw_passwd,
				itoa(pw->pw_uid)
		);
		if (ferror(fpTmp))
		  fatalErr(pwTmpFile);

		fprintf(fpTmp, "%s:%s:%s:%s\n",
				itoa(pw->pw_gid),
				pw->pw_gecos,
				pw->pw_dir,
				pw->pw_shell
		);
		if (ferror(fpTmp))
		  fatalErr(pwTmpFile);
	}
	endpwent();
	if (fflush(fpTmp) == EOF)
	  fatalErr(pwTmpFile);

	if (lseek(fdTmp, (off_t) 0, SEEK_SET) != 0)
	  fatalErr("Can't reread temp file");

	if ((fdPwd = open(shadow ? shadowFile : passwdFile, O_WRONLY | O_TRUNC)) < 0)
	  fatalErr("Can't recreate password file");

	while ((n = read(fdTmp, buf, sizeof(buf))) != 0) {
		if (n < 0 || write(fdPwd, buf, n) != n) {
			reportStdErr(prog, "Error rewriting password file, tell root!");
			exit(1);
		}
	}
	close(fdTmp);
	close(fdPwd);
	quit(0);
}




