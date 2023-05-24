#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/utsname.h>
#include <minix/minlib.h>

static char LOGIN[] = "/usr/bin/login";
static char SHELL[] = "/bin/sh";

static char *ttyName;	/* Name of the line */

/* Crude indication of a tty being physically secure: (see files.h)*/
#define secureTty(dev)	((unsigned) ((dev) - 0x0400) < (unsigned) 8)

/* Read one character from stdin. */
static int readChar() {
	int st;
	char ch1;

	/* read character from TTY */
	st = read(STDIN_FILENO, &ch1, 1);
	if (st == 0) {
		tell("\n");
		exit(EXIT_SUCCESS);
	} 
	if (st < 0) {
		tell("getty: ");
		tell(ttyName);
		tell(": read error\n");
		exit(EXIT_FAILURE);
	}
	return ch1 & 0xFF;
}

/* Handle the process of a GETTY. */
static void doGetty(char *name, size_t len, char **args, char *ttyName) {
	register char *np, *s, *s0;
	int ch;
	struct utsname utsName;
	char **banner, *t;
	static char *defBanner[] = { 
		"%s  Release %r Version %v  (%t)\n\n%n login: ", '\0' 
	};

	/* Clean up tty name. */
	if ((t = strrchr(ttyName, '/')))
	  ttyName = t + 1;

	/* Default banner? */
	if (args[0] == NULL)
	  args = defBanner;

	/* Display prompt. */
	ch = ' ';
	*name = '\0';
	while (ch != '\n') {
		/* Get data about this machine. */
		uname(&utsName);

		/* Print the banner. */
		for (banner = args; *banner != NULL; ++banner) {
			tell(banner == args ? "\n" : " ");
			s0 = *banner;
			for (s = *banner; *s != 0; ++s) {
				if (*s == '\\') {
					write(STDOUT_FILENO, s0, s - s0);
					s0 = s + 2;
					switch (*++s) {
						case 'n': tell("\n"); break;
						case 's': tell(" "); break;
						case 't': tell("\t"); break;
						case 0:   goto leave;
						default:  s0 = s;
					}
				} else if (*s == '%') {
					write(STDOUT_FILENO, s0, s - s0);
					s0 = s + 2;
					switch (*++s) {
						case 's': tell(utsName.sysname); break;
						case 'n': tell(utsName.nodename); break;
						case 'r': tell(utsName.release); break;
						case 'v': tell(utsName.version); break;
						case 'm': tell(utsName.machine); break;
						case 'p': tell(utsName.arch); break;
						case 't': tell(ttyName); break;
						case 0:   goto leave;
						default:  s0 = s - 1;
					}
				}
			}
leave:
			write(1, s0, s - s0);
		}

		np = name;
		while ((ch = readChar()) != '\n') {
			if (np < name + len)
			  *np++ = ch;
		}
		*np = '\0';
		if (*name == '\0')
		  ch = ' ';		/* Blank line typed! */
	}
}

/* Execute the login(1) command with the current
 * username as its argument. It will reply to the
 * calling user by typing "Password: "...
 */
static void doLogin(char *name) {
	struct stat st;

	execl(LOGIN, LOGIN, name, (char *) NULL);
	/* Failed to exec login. Impossible, but true. Try a shell, but only if
	 * the terminal is more or less secure, because it will be a root shell.
	 */
	tell("getty: can't exec ");
	tell(LOGIN);
	tell("\n");
	if (fstat(0, &st) == 0 && 
			S_ISCHR(st.st_mode) &&
			secureTty(st.st_rdev)) {
		execl(SHELL, SHELL, (char *) NULL);
	}
}

int main(int argc, char **argv) {
	char name[30];
	struct sigaction sa;

	/* Don't let QUIT dump core. */
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = exit;
	sigaction(SIGQUIT, &sa, NULL);

	ttyName = ttyname(STDIN_FILENO);
	if (ttyName == NULL) {
		tell("getty: tty name unknown\n");
		pause();
		return 1;
	}

	chown(ttyName, 0, 0);	/* Set owner of TTY to root */
	chmod(ttyName, 0600);	/* Mode to max secure */

	doGetty(name, sizeof(name), argv + 1, ttyName);		/* Handle getty() */
	name[29] = '\0';		/* Make sure the name fits! */

	doLogin(name);			/* and call login(1) if OK */

	return EXIT_FAILURE;	/* Never executed */
}
