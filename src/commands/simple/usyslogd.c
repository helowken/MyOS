#include <stdio.h>
#include <sys/select.h>
#include <sys/utsname.h>
#include <sys/types.h>
#include <time.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

static char *nodeName;

static void logLine(FILE *outFp, char *proc, char *line) {
	time_t now;
	struct tm *tm;
	char *d, *s;

	time(&now);
	tm = localtime(&now);
	d = asctime(tm);

	/* Trim off year and newline. */
	if ((s = strrchr(d, ' ')))
	  *s = '\0';
	if ((s = strchr(d, ' ')))
	  d = s + 1;
	fprintf(outFp, "%s %s: %s\n", d, nodeName, line);
}

static void copy(int inFd, FILE *outFp) {
	static char lineBuf[5 * 1024];
	int l, acc = 0;
	while ((l = read(inFd, lineBuf, sizeof(lineBuf) - 2)) > 0) {	/* 2 for '\n\0' */
		char *b, *eol;
		int i;
		acc += l;
		for (i = 0; i < l; ++i) {
			if (lineBuf[i] == '\0')
			  lineBuf[i] = ' ';
		}
		if (lineBuf[l - 1] == '\n')
		  --l;
		lineBuf[l] = '\n';
		lineBuf[l + 1] = '\0';
		b = lineBuf;
		while ((eol = strchr(b, '\n'))) {
			*eol = '\0';
			logLine(outFp, "kernel", b);
			b = eol + 1;
		}
	}

	/* Nothing sensible happended? Avoid busy-looping. */
	if (! acc)
	  sleep(1);
}

int main(int argc, char **argv) {
	int klogFd, n, maxFd;
	char *nn;
	FILE *logFp;
	struct utsname utsName;

	if (uname(&utsName) < 0) {
		perror("uname");
		return 1;
	}

	nodeName = utsName.nodename;
	if ((nn = strchr(nodeName, '.')))
	  *nn = '\0';

	if ((klogFd = open("/dev/klog", O_NONBLOCK | O_RDONLY)) < 0) {
		perror("/dev/klog");
		return 1;
	}

	if ((logFp = fopen("/usr/log/messages", "a")) == NULL) 
	  return 1;

	maxFd = klogFd;

	while (1) {
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(klogFd, &fds);
		n = select(maxFd + 1, &fds, NULL, NULL, NULL);
		if (n <= 0) {
			sleep(1);
			continue;
		}
		if (FD_ISSET(klogFd, &fds)) {
			copy(klogFd, logFp);
		}
		fflush(logFp);
		sync();
	}

	return 0;
}
