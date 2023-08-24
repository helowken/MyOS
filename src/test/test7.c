#include "util.h"

#define ITERATIONS	4
#define ITEMS	32
#define READ	10
#define	WRITE	20
#define UNLOCK	30
#define	U		70
#define L		80

char buf[ITEMS] = {0,1,2,3,4,5,6,7,8,9,8,7,6,5,4,3,2,1,0,1,2,3,4,5,6,7,8,9};
extern char **environ;
static int xfd;

static void cloexecTest() {
/* To test whether the FD_CLOEXEC flag actually causes files to be
 * closed upon exec, we have to exec something. The test is carried
 * out by forking, and then having the child exec test7 itself, but
 * with argument 0. This is detected, and control comes here.
 * File descriptors 3 and 10 should be closed here, and 6 open.
 */
	if (close(3) == 0) e(1001);		/* Close should fail; it was close on exec */
	if (close(6) != 0) e(1002);		/* CLose should succeed */
	if (close(10) == 0) e(1003);	/* Close should fail */
	fflush(stdout);
	exit(EXIT_SUCCESS);
}

static void test7a() {
/* Test pipe(). */
	int i, fd[2], ect;
	char buf2[ITEMS + 1];

	/* Create a pipe, write on it, and read it back. */
	subTest = 1;
	if (pipe(fd) != 0) e(1);
	if (write(fd[1], buf, ITEMS) != ITEMS) e(2);
	buf2[0] = 0;
	if (read(fd[0], buf2, ITEMS + 1) != ITEMS) e(3);
	ect = 0;
	for (i = 0; i < ITEMS; ++i) {
		if (buf[i] != buf2[i]) ++ect;
	}
	if (ect != 0) e(4);
	if (close(fd[0]) != 0) e(5);
	if (close(fd[1]) != 0) e(6);

	/* Error test. Keep opening pipes until it fails. Check error code. */
	errno = 0;
	while (1) {
		if (pipe(fd) < 0) break;
	}
	if (errno != EMFILE) e(7);

	/* Close all the pipes. */
	for (i = 3; i < OPEN_MAX; ++i) {
		close(i);
	}
}

static void test7b() {
/* Test mkfifo(). */
	int fdr, fdw, status;
	char buf2[ITEMS + 1];

	/* Create a fifo, write on it, and read it back. */
	subTest = 2;
	if (mkfifo("T7.b", 0777) != 0) e(1);
	switch (fork()) {
		case -1:
			forkFailed();
		case 0:
			/* Child reads from the fifo. */
			if ((fdr = open("T7.b", O_RDONLY)) < 0) e(5);
			if (read(fdr, buf2, ITEMS + 1) != ITEMS) e(6);
			if (strcmp(buf, buf2) != 0) e(7);
			if (close(fdr) != 0) e(8);
			exit(EXIT_SUCCESS);
		default:
			/* Parent writes on the fifo. */
			if ((fdw = open("T7.b", O_WRONLY)) < 0) e(2);
			if (write(fdw, buf, ITEMS) != ITEMS) e(3);
			wait(&status);
			if (close(fdw) != 0) e(4);
	}
	/* Check some error conditions. */
	if (mkfifo("T7.b", 0777) != -1) e(9);
	errno = 0;
	if (mkfifo("a/b/c", 0777) != -1) e(10);
	if (errno != ENOENT) e(11);
	errno = 0;
	if (mkfifo("", 0777) != -1) e(12);
	if (errno != ENOENT) e(13);
	errno = 0;
	if (mkfifo("T7.b/x", 0777) != -1) e(14);
	if (errno != ENOTDIR) e(15);
	if (unlink("T7.b") != 0) e(16);

	/* Now check fifos and the O_NONBLOCK flag. */
	if (mkfifo("T7.b", 0600) != 0) e(17);
	errno = 0;
	if (open("T7.b", O_WRONLY | O_NONBLOCK) != -1) e(18);
	if (errno != ENXIO) e(19);
	if ((fdr = open("T7.b", O_RDONLY | O_NONBLOCK)) < 0) e(20);
	if (fork()) {
		/* Parent reads from fdr. */
		wait(&status);		/* But first make sure writer has already run */
		if (WEXITSTATUS(status) != 77) e(21);
		if (read(fdr, buf2, ITEMS + 1) != ITEMS) e(22);
		if (strcmp(buf, buf2) != 0) e(23);
		if (close(fdr) != 0) e(24);
	} else {
		/* Child opens the fifo for writing and writes to it. */
		if ((fdw = open("T7.b", O_WRONLY | O_NONBLOCK)) < 0) e(25);
		if (write(fdw, buf, ITEMS) != ITEMS) e(26);
		if (close(fdw) != 0) e(27);
		exit(77);
	}
	
	if (unlink("T7.b") != 0) e(28);
}

static void test7c() {
/* Test fcntl(). */
	int fd, m, s, newFd, newFd2;
	struct stat stat1, stat2, stat3;

	subTest = 3;
	errno = -100;
	if ((fd = creat("T7.c", 0777)) < 0) e(1);

	/* Turn the per-file-descriptor flags on and off. */
	if (fcntl(fd, F_GETFD) != 0) e(2);	/* FD_CLOEXEC is initially off */
	if (fcntl(fd, F_SETFD, FD_CLOEXEC) != 0) e(3);	/* Turn it on */
	if (fcntl(fd, F_GETFD) != FD_CLOEXEC) e(4);	
	if (fcntl(fd, F_SETFD, 0) != 0) e(5);	
	if (fcntl(fd, F_GETFD) != 0) e(6);	

	/* Turn the open-file-description flags on and off. Start with O_APPEND. */
	m = O_WRONLY;	/* See creat() in fs */
	if (fcntl(fd, F_GETFL) != m) e(7);	/* See F_GETFL in fs */
	if (fcntl(fd, F_SETFL, O_APPEND) != 0) e(8);	/* See F_SETFL in fs */
	if (fcntl(fd, F_GETFL) != (O_APPEND | m)) e(9);
	if (fcntl(fd, F_SETFL, 0) != 0) e(10);	
	if (fcntl(fd, F_GETFL) != m) e(11);	

	/* Turn the open-file-description flags on and off. Now try O_NONBLOCK. */
	if (fcntl(fd, F_SETFL, O_NONBLOCK) != 0) e(12);	
	if (fcntl(fd, F_GETFL) != (O_NONBLOCK | m)) e(13);
	if (fcntl(fd, F_SETFL, 0) != 0) e(14);	
	if (fcntl(fd, F_GETFL) != m) e(15);	

	/* Now both at once. */
	if (fcntl(fd, F_SETFL, O_APPEND | O_NONBLOCK) != 0) e(16);	
	if (fcntl(fd, F_GETFL) != (O_APPEND | O_NONBLOCK | m)) e(17);
	if (fcntl(fd, F_SETFL, 0) != 0) e(18);	
	if (fcntl(fd, F_GETFL) != m) e(19);	
	
	/* Now test F_DUPFD. */
	if ((newFd = fcntl(fd, F_DUPFD, 0)) != 4) e(20);	/* 0-4 open */
	if ((newFd2 = fcntl(fd, F_DUPFD, 0)) != 5) e(21);
	if (close(newFd) != 0) e(22);
	if ((newFd = fcntl(fd, F_DUPFD, 0)) != 4) e(23);
	if (close(newFd) != 0) e(24);
	if ((newFd = fcntl(fd, F_DUPFD, 5)) != 6) e(25);	/* newFd = 6 */
	if (close(newFd2) != 0) e(26);

	/* O_APPEND should be inherited, but FD_CLOEXEC should be cleared. Check. */
	if (fcntl(fd, F_SETFD, FD_CLOEXEC) != 0) e(27);	
	if (fcntl(fd, F_SETFL, O_APPEND) != 0) e(28);
	if ((newFd2 = fcntl(fd, F_DUPFD, 10)) != 10) e(29);	/* newFd2 = 10 */
	if (fcntl(newFd2, F_GETFD) != 0) e(30);
	if (fcntl(newFd2, F_GETFL) != (O_APPEND | m)) e(31);
	if (fcntl(fd, F_SETFD, 0) != 0) e(32);

	/* Check if newFd and newFd2 are the same inode. */
	if (fstat(fd, &stat1) != 0) e(33);
	if (fstat(newFd, &stat2) != 0) e(34);
	if (fstat(newFd2, &stat3) != 0) e(35);
	if (stat1.st_dev != stat2.st_dev) e(36);
	if (stat1.st_dev != stat3.st_dev) e(37);
	if (stat1.st_ino != stat2.st_ino) e(38);
	if (stat1.st_ino != stat3.st_ino) e(39);

	/* Now check on the FD_CLOEXEC flag. Set it for fd (3) and newFd2 (10) */
	if (fd != 3 || newFd2 != 10 || newFd != 6) e(40);
	if (fcntl(fd, F_SETFD, FD_CLOEXEC) != 0) e(41);
	if (fcntl(newFd2, F_SETFD, FD_CLOEXEC) != 0) e(42);
	if (fcntl(newFd, F_SETFD, 0) != 0) e(43);
	if (fork()) {
		wait(&s);	/* Parent just waits */
		if (WEXITSTATUS(s) != 0) e(44);
	} else {
		execle("../test7", "test7", "0", (char *) 0, environ);
		exit(EXIT_FAILURE);
	}

	/* Finally, close all the files. */
	if (fcntl(fd, F_SETFD, 0) != 0) e(45);
	if (fcntl(newFd2, F_SETFD, 0) != 0) e(46);
	if (close(fd) != 0) e(47);
	if (close(newFd) != 0) e(48);
	if (close(newFd2) != 0) e(49);
}

static int set2(int how, int funcCode, int first, int last) {
	int r;
	struct flock flock;

	if (how == READ) flock.l_type = F_RDLCK;
	if (how == WRITE) flock.l_type = F_WRLCK;
	if (how == UNLOCK) flock.l_type = F_UNLCK;
	flock.l_whence = SEEK_SET;
	flock.l_start = (long) first;
	flock.l_len = (long) last - (long) first + 1;
	r = fcntl(xfd, funcCode, &flock);
	if (r != -1)
	  return 0;
	return -1;
}

static int set(int how, int first, int last) {
	return set2(how, F_SETLK, first, last);
}

static int locked(int b) {
/* Test to see if byte b is locked. Return L or U */
	struct flock flock;
	pid_t pid;
	int status;

	flock.l_type = F_WRLCK;
	flock.l_whence = SEEK_SET;
	flock.l_start = (long) b;
	flock.l_len = 1;

	/* Process' own locks are invisible to F_GETLK, so fork a child to test. */
	pid = fork();
	if (pid == 0) {
		int r;

		if (fcntl(xfd, F_GETLK, &flock) != 0) e(2000);
		r = flock.l_type == F_UNLCK ? U : L;
		if (r == L) {
			if (set(WRITE, b, b) == 0) e(3000);
			if (errno != EAGAIN) e(3001);
		}
		exit(r);
	}
	if (pid == -1) e(2001);
	if (fcntl(xfd, F_GETLK, &flock) != 0) e(2002);
	if (flock.l_type != F_UNLCK) e(2003);
	if (wait(&status) != pid) e(2004);
	if (! WIFEXITED(status)) e(2005);
	return WEXITSTATUS(status);
}

static void test7d() {
/* Test file locking. */
	subTest = 4;

	if ((xfd = creat("T7.d", 0777)) != 3) e(1);
	close(xfd);
	if ((xfd = open("T7.d", O_RDWR)) < 0) e(2);
	if (write(xfd, buf, ITEMS) != ITEMS) e(3);
	if (set(WRITE, 0, 3) != 0) e(4);
	if (set(WRITE, 5, 9) != 0) e(5);
	if (set(UNLOCK, 0, 3) != 0) e(6);
	if (set(UNLOCK, 4, 9) != 0) e(7);

	if (set(READ, 1, 4) != 0) e(8);
	if (set(READ, 4, 7) != 0) e(9);
	if (set(UNLOCK, 4, 7) != 0) e(10);
	if (set(UNLOCK, 1, 4) != 0) e(11);

	if (set(WRITE, 0, 3) != 0) e(12);
	if (set(WRITE, 5, 7) != 0) e(13);
	if (set(WRITE, 9, 10) != 0) e(14);
	if (set(UNLOCK, 0, 4) != 0) e(15);
	if (set(UNLOCK, 0, 7) != 0) e(16);
	if (set(UNLOCK, 0, 2000) != 0) e(17);

	if (set(WRITE, 0, 3) != 0) e(18);
	if (set(WRITE, 5, 7) != 0) e(19);
	if (set(WRITE, 9, 10) != 0) e(20);
	if (set(UNLOCK, 0, 100) != 0) e(21);

	if (set(WRITE, 0, 9) != 0) e(22);
	if (set(UNLOCK, 8, 9) != 0) e(23);
	if (set(UNLOCK, 0, 2) != 0) e(24);
	if (set(UNLOCK, 5, 5) != 0) e(25);
	if (set(UNLOCK, 4, 6) != 0) e(26);
	if (set(UNLOCK, 3, 3) != 0) e(27);
	if (set(UNLOCK, 7, 7) != 0) e(28);

	if (set(WRITE, 0, 10) != 0) e(29);
	if (set(UNLOCK, 0, 1000) != 0) e(30);

	/* Up until now, all locks have been disjoint. Now try conflicts. */
	if (set(WRITE, 0, 4) != 0) e(31);
	if (set(WRITE, 4, 7) != 0) e(32);
	if (set(WRITE, 5, 10) != 0) e(33);
	if (set(UNLOCK, 0, 11) != 0) e(34);

	/* File is now unlocked. Length 0 means whole file. */
	if (set(WRITE, 2, 1) != 0) e(35);
	if (set(WRITE, 9, 10) != 0) e(36);
	if (set(WRITE, 3, 3) != 0) e(37);
	if (set(UNLOCK, 0, -1) != 0) e(38);

	/* Test F_GETLK. */
	if (set(WRITE, 2, 3) != 0) e(39);
	if (locked(1) != U) e(40);	
	if (locked(2) != L) e(41);	
	if (locked(3) != L) e(42);	
	if (locked(4) != U) e(43);	
	if (set(UNLOCK, 2, 3) != 0) e(44);
	if (locked(2) != U) e(45);	
	if (locked(3) != U) e(46);	

	close(xfd);
}

static void test7e() {
/* Test to see if SETLKW blocks as it should. */
	int pid, s;
	
	subTest = 5;

	if ((xfd = creat("T7.e", 0777)) != 3) e(1);
	if (close(xfd) != 0) e(2);
	if ((xfd = open("T7.e", O_RDWR)) < 0) e(3);
	if (write(xfd, buf, ITEMS) != ITEMS) e(4);
	if (set(WRITE, 0, 3) != 0) e(5);

	if ((pid = fork())) {
		if (pid < 0) forkFailed();
		/* Parent waits until child has started before signaling it. */
		while (access("T7.e1", F_OK) != 0) {	/* File not exists */
		}
		unlink("T7.e1");
		sleep(1);
		if (kill(pid, SIGKILL) < 0) e(6);
		if (wait(&s) != pid) e(7);
	} else {
		/* Child tries to lock and should block. */
		if (creat("T7.e1", 0777) < 0) e(8);
		if (set2(WRITE, F_SETLKW, 0, 3) != 0) e(9);	/* Should block */
		errno = -1000;
		e(10);		/* Process should be killed by signal */
		exit(0);	/* Should never happen */

	}
	close(xfd);
}

static void sigFunc(int s) {
}

static void test7f() {
/* Test to see if SETLKW gives EINTR when interrupted. */
	int pid, s;

	subTest = 6;

	if ((xfd = creat("T7.f", 0777)) != 3) e(1);
	if (close(xfd) != 0) e(2);
	if ((xfd = open("T7.f", O_RDWR)) < 0) e(3);
	if (write(xfd, buf, ITEMS) != ITEMS) e(4);
	if (set(WRITE, 0, 3) != 0) e(5);

	if ((pid = fork())) {
		if (pid < 0) forkFailed();
		/* Parent waits until child has started before signaling it. */
		while (access("T7.f1", F_OK) != 0) {	/* File not exists */
		}
		unlink("T7.f1");
		sleep(1);
		if (kill(pid, SIGTERM) < 0) e(6);
		if (wait(&s) != pid) e(7);
		if (WEXITSTATUS(s) != 19) e(8);
	} else {
		/* Child tries to lock and should block.
		 * `signal(SIGTERM, sigFunc);' to set the signal handler is inadequate
		 * because on systems like BSD the sigaction flags for signal include
		 * `SA_RESTART' so syscalls are restarted after they have been
		 * interrupted by a signal.
		 */
		struct sigaction sa, osa;

		sa.sa_handler = sigFunc;
		sigemptyset(&sa.sa_mask);
		sa.sa_flags = 0;
		if (sigaction(SIGTERM, &sa, &osa) < 0) e(999);
		if (creat("T7.f1", 0777) < 0) e(9);
		if (set2(WRITE, F_SETLKW, 0, 3) != -1) e(10);	/* Should block */
		if (errno != EINTR) e(11);	/* Signal should release it */
		exit(19);
	}
	close(xfd);
}

static void test7g() {
/* Test to see if SETLKW unlocks when the needed lock becomes available. */
	int pid, s;

	subTest = 7;
	if ((xfd = creat("T7.g", 0777)) != 3) e(1);
	if (close(xfd) != 0) e(2);
	if ((xfd = open("T7.g", O_RDWR)) < 0) e(3);
	if (write(xfd, buf, ITEMS) != ITEMS) e(4);
	if (set(WRITE, 0, 3) != 0) e(5);

	if ((pid = fork())) {
		if (pid < 0) forkFailed();
		/* Parent waits until child to start. */
		while (access("T7.g1", F_OK) != 0) {	/* File not exists */
		}
		unlink("T7.g1");
		sleep(1);
		if (set(UNLOCK, 0, 3) != 0) e(5);
		if (wait(&s) != pid) e(6);
		if (WEXITSTATUS(s) != 29) e(7);
	} else {
		/* Child tells parent it is alive, then tries to lock and is blocked. */
		if (creat("T7.g1", 0777) < 0) e(8);
		if (set2(WRITE, F_SETLKW, 3, 3) != 0) e(9);	/* Process must block now */
		if (set(UNLOCK, 3, 3) != 0) e(10);
		exit(29);
	}
	close(xfd);
}

static void test7h() {
/* Test to see what happens if two processes block on the same lock. */
	int pid, pid2, s, w;

	subTest = 8;
	if ((xfd = creat("T7.h", 0777)) != 3) e(1);
	if (close(xfd) != 0) e(2);
	if ((xfd = open("T7.h", O_RDWR)) < 0) e(3);
	if (write(xfd, buf, ITEMS) != ITEMS) e(4);
	if (set(WRITE, 0, 3) != 0) e(5);

	if ((pid = fork())) {
		if (pid < 0) forkFailed();
		if ((pid2 = fork())) {
			if (pid2 < 0) forkFailed();
			/* Parent waits for child to start. */
			while (access("T7.h1", F_OK) != 0) {
			}
			while (access("T7.h2", F_OK) != 0) {
			}
			unlink("T7.h1");
			unlink("T7.h2");
			sleep(1);
			if (set(UNLOCK, 0, 3) != 0) e(6);

			w = wait(&s);
			if (w != pid && w != pid2) e(7);
			s = WEXITSTATUS(s);
			if (s != 39 && s != 49) e(8);

			w = wait(&s);
			if (w != pid && w != pid2) e(9);
			s = WEXITSTATUS(s);
			if (s != 39 && s != 49) e(10);
		} else {
			if (creat("T7.h1", 0777) < 0) e(11);
			if (set2(WRITE, F_SETLKW, 0, 0) != 0) e(12);	/* Block now */
			if (set(UNLOCK, 0, 0) != 0) e(13);
			exit(39);
		}
	} else {
		/* Child tells parent it is alive, then tries to lock and is blocked. */
		if (creat("T7.h2", 0777) < 0) e(14);
		if (set2(WRITE, F_SETLKW, 0, 1) != 0) e(15);	/* Must block now */
		if (set(UNLOCK, 0, 1) != 0) e(16);
		exit(49);
	}
	close(xfd);
}

static void test7i() {
/* Check error conditions for fcntl(). */
	int tfd, i;

	subTest = 9;

	errno = 0;
	if ((xfd = creat("T7.i", 0777)) != 3) e(1);
	if (close(xfd) != 0) e(2);
	if ((xfd = open("T7.i", O_RDWR)) < 0) e(3);
	if (write(xfd, buf, ITEMS) != ITEMS) e(4);
	if (set(WRITE, 0, 3) != 0) e(5);
	if (set(WRITE, 0, 0) != 0) e(6);
	if (errno != 0) e(7);

	errno = 0;
	if (set(WRITE, 3, 3) != 0) e(8);
	if (errno != 0) e(9);
	tfd = xfd;
	xfd = -99;
	errno = 0;
	if (set(WRITE, 0, 0) != -1) e(10);
	if (errno != EBADF) e(11);

	errno = 0;
	if ((xfd = open("T7.i", O_WRONLY)) < 0) e(12);
	if (set(READ, 0, 0) != -1) e(13);
	if (errno != EBADF) e(14);
	if (close(xfd) != 0) e(15);

	errno = 0;
	if ((xfd = open("T7.i", O_RDONLY)) < 0) e(16);
	if (set(WRITE, 0, 0) != -1) e(17);
	if (errno != EBADF) e(18);
	if (close(xfd) != 0) e(19);
	xfd = tfd;

	/* Check for EINVAL. */
	errno = 0;
	if (fcntl(xfd, F_DUPFD, OPEN_MAX) != -1) e(20);
	if (errno != EINVAL) e(21);
	errno = 0;
	if (fcntl(xfd, F_DUPFD, -1) != -1) e(22);
	if (errno != EINVAL) e(23);

	xfd = 0;	/* stdin does not support locking */
	errno = 0;
	if (set(READ, 0, 0) != -1) e(24);
	if (errno != EINVAL) e(25);
	xfd = tfd;

	/* Check ENOLCK. */
	for (i = 0; i < ITEMS; ++i) {
		if (set(WRITE, i, i) == 0) continue;
		if (errno != ENOLCK) {
			e(26);
			break;
		}
	}

	/* Check EMFILE. */
	for (i = xfd + 1; i < OPEN_MAX; ++i) {
		open("T7.i", 0);	/* Use up all fds. */
	}
	errno = 0;
	if (fcntl(xfd, F_DUPFD, 0) != -1) e(27);
	if (errno != EMFILE) e(28);

	for (i = xfd; i < OPEN_MAX; ++i) {
		if (close(i) != 0)
		  e(29);
	}
}

static void test7j() {
/* Test file locking with two processes. */
	int s;

	subTest = 10;

	if ((xfd = creat("T7.j", 0777)) != 3) e(1);
	close(xfd);
	if ((xfd = open("T7.j", O_RDWR)) < 0) e(2);
	if (write(xfd, buf, ITEMS) != ITEMS) e(3);
	if (set(WRITE, 0, 4) != 0) e(4);
	if (set(READ, 10, 16) != 0) e(5);

	/* Up until now, all locks have been disjoint. Now try conflicts. */
	if (fork()) {
		/* Parent just waits for child to finish. */
		wait(&s);
	} else {
		/* Child does the testing. */
		errno = -100;
		if (set(WRITE, 5, 7) < 0) e(6);
		if (set(WRITE, 4, 7) >= 0) e(7);	/* Child may not lock byte 4 */
		if (errno != EACCES && errno != EAGAIN) e(8);
		if (set(WRITE, 5, 9) != 0) e(9);
		if (set(UNLOCK, 5, 9) != 0) e(10);
		if (set(READ, 9, 17) < 0) e(11);	/* Shared READ lock is ok */
		exit(0);
	}
	close(xfd);
}

void main(int argc, char **argv) {
	int i;

	if (strcmp(argv[1], "0") == 0)
	  cloexecTest();	/* Important; do not remove this! */

	setup(argc, argv);

	for (i = 0; i < ITERATIONS; ++i) {
		if (m &00001) test7a();
		if (m &00002) test7b();
		if (m &00004) test7c();
		if (m &00010) test7d();
		if (m &00020) test7e();
		if (m &00040) test7f();
		if (m &00100) test7g();
		if (m &00200) test7h();
		if (m &00400) test7i();
		if (m &01000) test7j();
	}
	quit();
}
