#include "util.h"

#define ITERATIONS	5

int is, array[4], cumSig, sigCnt;
char buf[2048];
int iteration, kk = 0;


static void test2a() {
/* Test pipes */
	int fd[2];
	int n, i, j, q = 0;

	subTest = 1;
	if (pipe(fd) < 0) 
	  fail("pipe error.  errno=%d\n", errno);
	i = fork();
	if (i < 0) 
	  fail("fork failed\n");
	if (i != 0) {
		/* Parent */
		close(fd[0]);
		for (i = 0; i < 2048; ++i) {
			buf[i] = i & 0377;
		}
		for (q = 0; q < 8; ++q) {
			if (write(fd[1], buf, 2048) < 0) 
			  fail("write pipe err.  errno=%d\n", errno);
		}
		close(fd[1]);
		wait(&q);
		if (q != (58 << 8))		/* Exit status of wait */
		  fail("wrong exit code %d\n", q);	
	} else {
		/* Child */
		close(fd[1]);
		for (q = 0; q < 32; ++q) {
			n = read(fd[0], buf, 512);
			if (n != 512) 
			  fail("read yielded %d bytes, not 512\n", n);

			for (j = 0; j < n; ++j) {
				if ((buf[j] & 0377) != (kk & 0377)) 
				  printf("wrong data: %d %d %d \n ",
							  j, buf[j] & 0377, kk & 0377);
				else
				  ++kk;

			}
		}
		exit(58);
	}
}

void sigPipe(int s) {
	++sigCnt;
	++cumSig;
}

static void test2b() {
	int fd[2], n;
	char buf[4];

	subTest = 2;
	sigCnt = 0;
	signal(SIGPIPE, sigPipe);
	pipe(fd);
	if (fork()) {	/* Parent */
		close(fd[0]);
		while (sigCnt == 0) {
			write(fd[1], buf, 1);
		}
		wait(&n);
	} else {	/* Child */
		close(fd[0]);
		close(fd[1]);
		exit(EXIT_SUCCESS);
	}
}

static void test2c() {
	int n;

	subTest = 3;
	signal(SIGINT, SIG_DFL);	/* Make child exit */
	is = 0;
	if ((array[is++] = fork()) > 0) {	/* Parent */
		if ((array[is++] = fork()) > 0) {	/* Parent */
			if ((array[is++] = fork()) > 0) {	/* Parent */
				if ((array[is++] = fork()) > 0) {	/* Parent */
					signal(SIGINT, SIG_IGN);
					kill(array[0], SIGINT);
					kill(array[1], SIGINT);
					kill(array[2], SIGINT);
					kill(array[3], SIGINT);
					wait(&n);
					wait(&n);
					wait(&n);
					wait(&n);
				} else {
					pause();
				}
			} else {
				pause();
			}
		} else {
			pause();
		} 
	} else {
		pause();
	}
}

static void test2d() {
	int pid, statLoc, s;

	/* Test waitpid. */
	subTest = 4;

	/* Test waitpid(pid, arg2, 0) */
	pid = fork();
	if (pid < 0)
	  e(1);
	if (pid > 0) {	/* Parent */
		s = waitpid(pid, &statLoc, 0);
		if (s != pid)
		  e(2);
		if (! WIFEXITED(statLoc))
		  e(3);
		if (WIFSIGNALED(statLoc))
		  e(4);
		if (WEXITSTATUS(statLoc) != 22)
		  e(5);
	} else {	/* Child */
		exit(22);
	}

	/* Test waitpid(-1, arg2, 0) */
	pid = fork();
	if (pid < 0)
	  e(6);
	if (pid > 0) {	/* Parent */
		s = waitpid(-1, &statLoc, 0);
		if (s != pid)
		  e(7);
		if (! WIFEXITED(statLoc))
		  e(8);
		if (WIFSIGNALED(statLoc))
		  e(9);
		if (WEXITSTATUS(statLoc) != 33)
		  e(10);
	} else {	/* Child */
		exit(33);
	}

	/* Test waitpid(0, arg2, 0) */
	pid = fork();
	if (pid < 0)
	  e(11);
	if (pid > 0) {	/* Parent */
		s = waitpid(0, &statLoc, 0);
		if (s != pid)
		  e(12);
		if (! WIFEXITED(statLoc))
		  e(13);
		if (WIFSIGNALED(statLoc))
		  e(14);
		if (WEXITSTATUS(statLoc) != 44)
		  e(15);
	} else {	/* Child */
		exit(44);
	}

	/* Test waitpid(0, arg2, WNOHANG) */
	signal(SIGTERM, SIG_DFL);
	pid = fork();
	if (pid < 0)
	  e(16);
	if (pid > 0) {	/* Parent */
		s = waitpid(0, &statLoc, WNOHANG);
		if (s != 0)
		  e(17);
		if (kill(pid, SIGTERM) != 0)
		  e(18);
		if (waitpid(pid, &statLoc, 0) != pid)
		  e(19);
		if (WIFEXITED(statLoc))
		  e(20);
		if (! WIFSIGNALED(statLoc))
		  e(21);
		if (WTERMSIG(statLoc) != SIGTERM)
		  e(22);
	} else {	/* Child */
		pause();
	}

	/* Test some error conditions. */
	errno = 9999;
	if (waitpid(0, &statLoc, 0) != -1)
	  e(23);
	if (errno != ECHILD)
	  e(24);

	errno = 9999;
	if (waitpid(0, &statLoc, WNOHANG) != -1)
	  e(25);
	if (errno != ECHILD)
	  e(26);
}

static void test2e() {
	int pid1, pid2, statLoc, s;

	/* Test waitpid with two children. */
	subTest = 5;
	if (iteration > 1)	/* Slow test, don't do it too much */
	  return;
	if ((pid1 = fork())) {	/* Parent */
		if ((pid2 = fork())) {	/* Parent, collect second child first.  */
			s = waitpid(pid2, &statLoc, 0);
			if (s != pid2)
			  e(1);
			if (! WIFEXITED(statLoc))
			  e(2);
			if (WIFSIGNALED(statLoc))
			  e(3);
			if (WEXITSTATUS(statLoc) != 222)
			  e(4);

			/* Now collect first child. */
			s = waitpid(pid1, &statLoc, 0);
			if (s != pid1)
			  e(5);
			if (! WIFEXITED(statLoc))
			  e(6);
			if (WIFSIGNALED(statLoc))
			  e(7);
			if (WEXITSTATUS(statLoc) != 111)
			  e(8);
		} else {
			/* Child 2. */
			sleep(2);		/* Child 2 delays before exiting. */
			exit(222);
		}
	} else {
		/* Child 1. */
		exit(111);		/* Child 1 exits immediately */
	}
}

static void test2f() {
/* Test getpid, getppid, getuid, etc. */
	int pid, pid1, ppid, cpid, statLoc, err;

	subTest = 6;
	errno = -2000;
	err = 0;
	pid = getpid();
	if ((pid1 = fork())) {	/* Parent */
		if (wait(&statLoc) != pid1)
		  e(1);
		if (WEXITSTATUS(statLoc) != (pid1 & 0377))
		  e(2);
	} else {	/* Child */
		cpid = getpid();
		ppid = getppid();
		if (ppid != pid)
		  err = 3;
		if (cpid == ppid)
		  err = 4;
		exit(cpid & 0377);
	}
	if (err != 0)
	  e(err);
}

static void test2g() {
/* Test time(), times() */
	time_t t1, t2;
	clock_t t3, t4;
	struct tms tmsBuf;

	subTest = 7;
	errno = -7000;

	/* First time(). */
	t1 = -1;
	t2 = -2;
	t1 = time(&t2);
	if (t1 < 650000000L)	/* 650000000 is Sept. 1990 */
	  e(1);	
	if (t1 != t2)
	  e(2);
	t1 = -1;
	t1 = time((time_t *) NULL);
	if (t1 < 650000000L)
	  e(3);
	t3 = times(&tmsBuf);
	sleep(1);
	t2 = time((time_t *) NULL);
	if (t2 < 0L)
	  e(4);
	if (t2 - t1 < 1)
	  e(5);

	/* Now times(). */
	t4 = times(&tmsBuf);
	if (t4 == (clock_t) -1) 
	  e(6);
	if (t4 - t3 < CLOCKS_PER_SEC)
	  e(7);
	if (tmsBuf.tms_utime < 0)
	  e(8);
	if (tmsBuf.tms_stime < 0)
	  e(9);
	if (tmsBuf.tms_cutime < 0)
	  e(10);
	if (tmsBuf.tms_cstime < 0)
	  e(11);
}

static void test2h() {
/* Test getgroups() */
	gid_t g[10];

	subTest = 8;
	errno = -8000;
	if (getgroups(10, g) != 0)
	  e(1);
	if (getgroups(1, g) != 0)
	  e(2);
	if (getgroups(0, g) != 0)
	  e(3);
}

void main(int argc, char **argv) {
	int i;

	setup(argc, argv);

	for (i = 0; i < ITERATIONS; ++i) {
		iteration = i;
		if (m & 0001) test2a();
		if (m & 0002) test2b();
		if (m & 0004) test2c();
		if (m & 0010) test2d();
		if (m & 0020) test2e();
		if (m & 0040) test2f();
		if (m & 0100) test2g();
		if (m & 0200) test2h();
	}
	subTest = 100;
	if (cumSig != ITERATIONS)
	  e(101);

	quit();
}
