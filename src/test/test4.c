#include "util.h"

static pid_t pid0, pid1, pid2, pid3;
static int s, i, fd, nextByte;
static char *tempFile = "test4.temp";
static char buf[1024];

static void subr() {
	if ((pid0 = fork()) != 0) {
		if (pid0 < 0)
		  forkFailed();
		if ((pid1 = fork()) != 0) {
			if (pid1 < 0) 
			  forkFailed();
			if ((pid2 = fork()) != 0) {
				if (pid2 < 0)
				  forkFailed();
				if ((pid3 = fork()) != 0) {
					if (pid3 < 0)
					  forkFailed();
					for (i = 0; i < 10000; ++i) {
					}
					kill(pid2, SIGKILL);
					kill(pid1, SIGKILL);
					kill(pid0, SIGKILL);
					wait(&s);
					wait(&s);
					wait(&s);
					wait(&s);
				} else {
					fd = open(tempFile, O_RDONLY);
					lseek(fd, 20480L * nextByte, SEEK_SET);
					for (i = 0; i < 10; ++i) {
						read(fd, buf, 1024);
					}
					++nextByte;
					close(fd);
					exit(EXIT_SUCCESS);
				}
			} else {
				while (1) {
					getpid();
				}
			}
		} else {
			while (1) {
				getpid();
			}
		}
	} else {
		while (1) {
			getpid();
		}
	}
}

void main(int argc, char **argv) {
	int k;

	setup(argc, argv);

	creat(tempFile, 0777);
	for (k = 0; k < 20; ++k) {
		subr();
	}
	unlink(tempFile);
	quit();
}

