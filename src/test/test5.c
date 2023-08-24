#include "util.h"

#define ITERATIONS	2

static int zero[1024];
static int sigMap[5] = {SIGKILL, SIGUSR1, SIGSEGV};

volatile int childSigs, parentSigs, alarms;

static void parent(int cpid) {
	int i, pid;

	for (i = 0; i < 3; ++i) {
		if (kill(cpid, SIGHUP) < 0) e(6);
		while (parentSigs == 0) {
		}
		--parentSigs;
	}
	if ((pid = wait(&i)) < 0) e(7);
	if (WEXITSTATUS(i) != 6) e(8);
}

static void child(int ppid) {
	int i;

	for (i = 0; i < 3; ++i) {
		while (childSigs == 0) {
		}
		--childSigs;
		if (kill(ppid, SIGUSR1) < 0) e(9);
	}
	exit(6);
}

static void func1(int s) {
	if (signal(SIGHUP, func1) == SIG_ERR) e(10);
	++childSigs;
}

static void func10(int s) {
	if (signal(SIGUSR1, func10) == SIG_ERR) e(11);
	++parentSigs;
}

static void test5a() {
	int ppid, cpid, flag, *zp;

	subTest = 0;
	flag = 0;
	for (zp = &zero[0]; zp < &zero[1024]; ++zp) {
		if (*zp != 0) {
			flag = 1;
			break;
		}
	}
	if (flag) e(0);		/* Check if bss is cleared to 0 */
	if (signal(SIGHUP, func1) == SIG_ERR) e(1);
	if (signal(SIGUSR1, func10) == SIG_ERR) e(2);
	ppid = getpid();
	if ((cpid = fork())) {	/* Parent */
		if (cpid < 0) 
		  forkFailed();
		parent(cpid);
	} else {	/* Child */
		child(ppid);
	}
	if (signal(SIGHUP, SIG_DFL) == SIG_ERR) e(4);
	if (signal(SIGUSR1, SIG_DFL) == SIG_ERR) e(5);
}

static void test5b() {
	int cpid, n, pid;

	subTest = 1;
	if ((pid = fork())) {
		if (pid < 0) forkFailed();
		if ((pid = fork())) {
			if (pid < 0) forkFailed();
			if ((cpid = fork())) {
				if (cpid < 0) forkFailed();
				if (kill(cpid, SIGKILL) < 0) e(12);
				if (wait(&n) < 0) e(13);
				if (wait(&n) < 0) e(14);
				if (wait(&n) < 0) e(15);
			} else {
				pause();
				while (1) {
				}
			}
		} else {
			exit(0);
		}
	} else {
		exit(0);
	}
}

static void test5c() {
	int n, i, pid, wpid;

	/* Test exit status codes for processes killed by signals. */
	subTest = 3;
	for (i = 0; i < 2; ++i) {
		if ((pid = fork())) {
			if (pid < 0) forkFailed();
			sleep(2);	/* Wait for child to pause */
			if (kill(pid, sigMap[i]) < 0) {
				e(20);
				exit(1);
			}
			if ((wpid = wait(&n)) < 0) e(21);
			if (WTERMSIG(n) != sigMap[i]) e(22);
			if (pid != wpid) e(23);
		} else {
			pause();
			exit(0);
		}
	}
}

static void funcAlarm(int s) {
	++alarms;
}

static void test5d() {
/* Test alarm */
	int i;

	subTest = 4;
	alarms = 0;
	for (i = 0; i < 8; ++i) {
		signal(SIGALRM, funcAlarm);
		alarm(1);
		pause();
		if (alarms != i + 1) e(24);
	}
}

static void func8() {
}

static void test5e() {
/* When a signal knocks a process out of WAIT or PAUSE, it is supposed to
 * get EINTR as error status. Check that.
 */
	int n;

	subTest = 5;
	if (signal(SIGFPE, func8) == SIG_ERR) e(25);
	if ((n = fork())) {
		/* Parent must delay to give child a chance to pause. */
		if (n < 0) forkFailed();
		sleep(1);
		if (kill(n, SIGFPE) < 0) e(26);
		if (wait(&n) < 0) e(27);
		if (signal(SIGFPE, SIG_DFL) == SIG_ERR) e(28);
	} else {
		pause();
		if (errno != EINTR) e(29);
		exit(0);
	}
}

static void test5f() {
	int i, j, k, n;

	subTest = 6;
	if (getuid() != 0) {
		printf("Not root user\n");
		return;
	}
	n = fork();
	if (n < 0) forkFailed();
	if (n) {
		wait(&i);
		i = WEXITSTATUS(i);
		if (i != n) e(30);	
	} else {
		i = getgid();
		j = getegid();
		k = (i + j + 7) & 0377;
		if (setgid(k) < 0) e(31);
		if (getgid() != k) e(32);
		if (getegid() != k) e(33);
		i = getuid();
		j = geteuid();
		k = (i + j + 1) & 0377;
		if (setuid(k) < 0) e(34);
		if (getuid() != k) e(35);
		if (geteuid() != k) e(36);
		i = getpid() & 0377;
		if (wait(&j) != -1) e(37);
		exit(i);	/* Child pid as the exit status */
	}
}

static void func11() {
	e(38);
}

static void test5g() {
	int n;

	subTest = 7;
	signal(SIGSEGV, func11);
	signal(SIGSEGV, SIG_IGN);
	n = getpid();
	if (kill(n, SIGSEGV) != 0) e(1);
	signal(SIGSEGV, SIG_DFL);
}

static void test5h() {
/* When a signal knocks a process out of PIPE, it is supposed to
 * get EINTR as error status. Check that.
 */
	int n, fd[2];
	char *file = "XXX.test5";

	subTest = 8;
	unlink(file);
	if (signal(SIGFPE, func8) == SIG_ERR) e(1);
	pipe(fd);
	if ((n = fork())) {
		if (n < 0) forkFailed();
		while (access(file, F_OK) != 0) {
		}
		sleep(1);
		unlink(file);
		if (kill(n, SIGFPE) < 0) e(2);
		if (wait(&n) < 0) e(3);
		if (signal(SIGFPE, SIG_DFL) == SIG_ERR) e(4);
		if (close(fd[0]) != 0) e(5);
		if (close(fd[1]) != 0) e(6);
	} else {
		if (creat(file, 0777) < 0) e(7);
		read(fd[0], (char *) &n, 1);
		if (errno != EINTR) e(8);
		exit(0);
	}
}

static void test5i() {
	int fd[2], pid, buf[10], n;
	char *file = "XXXxxxXXX";

	subTest = 9;
	pipe(fd);
	unlink(file);

	if ((pid = fork())) {
		while (access(file, F_OK) != 0) {
		}
		sleep(1);
		if (kill(pid, SIGKILL) != 0) e(1);
		if (wait(&n) < 0) e(2);
		if (close(fd[0]) != 0) e(3);
		if (close(fd[1]) != 0) e(4);
	} else {
		if (creat(file, 0777) < 0) e(5);
		read(fd[0], (char *) &buf, 1);
		e(5);	/* Should be killed by signal and not get here */
	}
	unlink(file);
}

void main(int argc, char **argv) {
	int i;

	setup(argc, argv);

	for (i = 0; i < ITERATIONS; ++i) {
		if (m & 0001) test5a();
		if (m & 0002) test5b();
		if (m & 0004) test5c();
		if (m & 0010) test5d();
		if (m & 0020) test5e();
		if (m & 0040) test5f();
		if (m & 0100) test5g();
		if (m & 0200) test5h();
		if (m & 0400) test5i();
	}
	quit();
}
