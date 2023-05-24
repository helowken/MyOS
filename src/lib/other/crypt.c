#define nil	((void *) 0)
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/wait.h>

/* Set-uid root program to read /etc/shadow or encrypt passwords. */
static char PWD_AUTH[] = "/usr/lib/pwdauth";
#define LEN	1024

static void tell(const char *s0, ...) {
	va_list ap;
	const char *s;

	va_start(ap, s0);
	s = s0;
	while (s != nil) {
		write(STDERR_FILENO, s, strlen(s));
		s = va_arg(ap, const char *);
	}
	va_end(ap);
}

char *crypt(const char *key, const char *salt) {
	pid_t pid;
	int status;
	int pfd[2];
	static char pwData[LEN];
	char *p = pwData;
	const char *k = key;
	const char *s = salt;
	int n;

	/* Fill pwData[] with the key and salt. */
	while ((*p++ = *k++) != 0) {
		if (p == pwData + LEN - 1)
		  goto fail;
	}
	while ((*p++ = *s++) != 0) {
		if (p == pwData + LEN)
		  goto fail;
	}

	if (pipe(pfd) < 0)
	  goto fail;

	/* Prefill the pipe. */
	write(pfd[1], pwData, p - pwData);

	switch ((pid = fork())) {
		case -1:
			close(pfd[0]);
			close(pfd[1]);
			goto fail;
		case 0:		/* Child */
			/* Connect both input and output to the pipe. */
			if (pfd[0] != STDIN_FILENO) {
				dup2(pfd[0], STDIN_FILENO);
				close(pfd[0]);
			}
			if (pfd[1] != STDOUT_FILENO) {
				dup2(pfd[1], STDOUT_FILENO);
				close(pfd[1]);
			}

			execl(PWD_AUTH, PWD_AUTH, (char *) nil);
			tell("crypt(): ", PWD_AUTH, ": ", strerror(errno), 
						"\r\n", (char *) nil);
			/* No pwdauth? Fail! */
			read(STDIN_FILENO, pwData, LEN);
			_exit(1);
	}
	close(pfd[1]);

	status = -1;
	while (waitpid(pid, &status, 0) == -1 && errno == EINTR) {
	}
	if (status != 0) {
		close(pfd[0]);
		goto fail;
	}

	/* Read and return the result. Check if it contains exactly 
	 * one string.
	 */
	n = read(pfd[0], pwData, LEN);
	close(pfd[0]);
	if (n < 0)
	  goto fail;
	p = pwData + n;
	n = 0;
	while (p > pwData) {
		if (*--p == 0)
		  ++n;
	}
	if (n != 1)
	  goto fail;
	return pwData;

fail:
	pwData[0] = salt[0] ^ 1;	/* Make result != salt */
	pwData[1] = 0;
	return pwData;
}
