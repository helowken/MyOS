#include "shell.h"
#include "var.h"
#include "nodes.h"
#include "jobs.h"
#include "options.h"
#include "output.h"
#include "redir.h"
#include "memalloc.h"
#include "exec.h"
#include "error.h"
#include "mystring.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "errno.h"
#include "unistd.h"

static char *currDir;	/* Current working directory. */
static char *cdCompPath;

#if UDIR || TILDE
extern int didUserDir;
#endif

/* Get the next component of the path name pointed to by cdCompPath.
 * This routine overwrites the string pointed to by cdCompPath.
 */
static char *getComponent() {
	register char *p;
	char *start;

	if ((p = cdCompPath) == NULL)
	  return NULL;
	start = cdCompPath;
	while (*p != '/' && *p != '\0') {
		++p;
	}
	if (*p == '\0')
	  cdCompPath = NULL;
	else {
		*p++ = '\0';
		cdCompPath = p;
	}
	return start;
}

/* Update currDir (the name of the current directory) in response to a
 * cd command. We also call hashCd to let the routines in exec.c know
 * that the current directory has changed.
 */
static void updatePwd(char *dir) {
	char *new;
	char *p;

	hashCd();	/* Update command hash table. */
	cdCompPath = stackAlloc(strlen(dir) + 1);
	scopy(dir, cdCompPath);
	START_STACK_STR(new);
	if (*dir != '/') {
		if (currDir == NULL)
		  return;
		p = currDir;
		while (*p) {
			ST_PUTC(*p++, new);
		}
		if (p[-1] == '/')
		  ST_UNPUTC(new);
	}
	while ((p = getComponent()) != NULL) {
		if (equal(p, "..")) {
			while (new > stackBlock()) {
				ST_UNPUTC(new);
				if (*new == '/')
				  break;
			} 
		} else if (*p != '\0' && ! equal(p, ".")) {
			ST_PUTC('/', new);
			while (*p) {
				ST_PUTC(*p++, new);
			}
		}
	}
	if (new == stackBlock())
	  ST_PUTC('/', new);
	STACK_STR_NUL(new);
	if (currDir)
	  ckFree(currDir);
	currDir = saveStr(stackBlock());
}

/* Actually do the chdir. If the name refers to symbolic links, we
 * compute the actual directory name before doing the cd. In an
 * interactive shell, print the directory name if "print" is nonzero or if the name refers to a symbolic link. We also print the name if "/u/logname" was expanded in it, since this is similar to a symbolic link. (The check for this breaks if the user gives the cd command some additional, unused arguments.)
 */
static int doCd(char *dest, int print, int toHome) {
	register char *p, *q;
	char *symlink;
	char *component;
	struct stat sb;
	int first;
	int i;

	if (didUserDir)
	  print = 1;

top:
	cdCompPath = dest;
	START_STACK_STR(p);
	if (*dest == '/') {
		ST_PUTC('/', p);
		++cdCompPath;
	}
	first = 1;
	while ((q = getComponent()) != NULL) {
		if (q[0] == '\0' || 
				(q[0] == '.' && q[1] == '\0'))
		  continue;
		if (! first)
		  ST_PUTC('/', p);
		first = 0;
		component = q;
		while (*q) {
			ST_PUTC(*q++, p);
		}
		if (equal(component, ".."))
		  continue;
		STACK_STR_NUL(p);
		if (lstat(stackBlock(), &sb) < 0)
		  error("lstat %s failed", stackBlock());
		if (! S_ISLNK(sb.st_mode))
		  continue;

		/* Hit a symbolic link. We have to start all over again. */
		print = 1;
		ST_PUTC('\0', p);
		/* Set p = link's realPath + ('/' + cdCompPath) */
		symlink = grabStackStr(p);
		i = (int) sb.st_size + 2;	/* 2 for '/' and '\0' */
		if (cdCompPath != NULL)
		  i += strlen(cdCompPath);
		p = stackAlloc(i);
		if (readlink(symlink, p, (int) sb.st_size) < 0)
		  error("readlink %s failed", symlink);
		if (cdCompPath != NULL) {
			p[(int) sb.st_size] = '/';
			scopy(cdCompPath, p + (int) sb.st_size + 1);
		} else {
			p[(int) sb.st_size] = '\0';
		}
		if (p[0] != '/') {	/* Relative path name */
			/* Set dest = link's dir + p */
			char *r;
			q = r = symlink;
			while (*q) {
				if (*q++ == '/')
				  r = q;
			}
			/* If symlink contains dir, then r > 0 
			 * Else r = 0
			 */
			*r = '\0';
			dest = stackAlloc(strlen(symlink) + strlen(p) + 1);
			scopy(symlink, dest);
			strcat(dest, p);
		} else {
			dest = p;
		}
		goto top;	
	}
	ST_PUTC('\0', p);
	p = grabStackStr(p);
	INTOFF;
	/* The empty string is not a legal argument to chdir on a POSIX 1003.1
	 * system. */
	if (p[0] != '\0' && chdir(p) < 0) {
		INTON;
		return -1;
	}
	updatePwd(p);
	INTON;
	if (print && ! toHome && iflag)
	  out1Format("%s\n", p);
	return 0;
}

/* Run /bin/pwd to find out what the current directory is. We suppress
 * interrupts throughout most of this, but the user can still break out
 * of it by killing the pwd program. If we already know the current
 * directory, this routine returns immediately.
 */

#define MAX_PWD	256

static void getPwd() {
	char buf[MAX_PWD];
	char *p;
	int i;
	int status;
	Job *jp;
	int pip[2];

	if (currDir) 
	  return;
	INTOFF;
	if (pipe(pip) < 0)
	  error("Pipe call failed");
	jp = makeJob(1);
	if (forkShell(jp, (Node *) NULL, FORK_NO_JOB) == 0) {	/* Child */
		close(pip[0]);
		copyToStdout(pip[1]);
		execl("/bin/pwd", "pwd", NULL);
		error("Cannot exec /bin/pwd");
	}
	/* Parent */
	close(pip[1]);
	pip[1] = -1;
	p = buf;
	while ((i = read(pip[0], p, buf + MAX_PWD - p)) > 0 ||
		   (i == -1 && errno == EINTR)) {
		if (i > 0)
		  p += i;
	}
	close(pip[0]);
	pip[0] = -1;
	status = waitForJob(jp);
	if (status != 0)
	  error(NULL);
	if (i < 0 || p == buf || p[-1] != '\n')
	  error("pwd command failed");
	p[-1] = '\0';
	currDir = saveStr(buf);
	INTON;
}

int cdCmd(int argc, char **argv) {
	char *dest;
	char *path;
	char *p;
	struct stat sb;
	int toHome = 0;
	
	nextOpt(nullStr);
	if ((dest = *argPtr) == NULL) {
		if ((dest = bltinLookup("HOME", 1)) == NULL)
		  error("HOME not set");
		toHome = 1;
	}
	if (*dest == '/' || (path = bltinLookup("CDPATH", 1)) == NULL) 
	  path = nullStr;
	while ((p = pathAdvance(&path, dest)) != NULL) {
		if (stat(p, &sb) >= 0 && 
			S_ISDIR(sb.st_mode) &&
			doCd(p, strcmp(p, dest), toHome) >= 0)
		  return 0;
	}
	error("can't cd to %s\n", dest);
	return 1;
}

int pwdCmd(int argc, char **argv) {
	getPwd();
	out1Str(currDir);
	out1Char('\n');
	return 0;
}
