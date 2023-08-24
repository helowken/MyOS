#include "util.h"

#define System(cmd)		if (system(cmd) != 0) printf("``%s'' failed\n", cmd)
#define Chdir(dir)		if (chdir(dir) != 0) printf("Can't goto %s\n", dir)
#define Stat(a,b)		if (stat(a,b) != 0) printf("Can't stat %s\n", a)

int mode(char *arg) {
	struct stat st;
	Stat(arg, &st);
	return st.st_mode & 0777;
}

static int umode(char *arg) {
	return 0777 ^ mode(arg);
}

static void test22a() {
	int fd1, fd2;
	int i, oldMask;
	int status;

	subTest = 1;

	system("chmod 777 ../DIR_22/* ../DIR_22/*/* > /dev/null 2>&1");
	System("rm -rf ../DIR_22/*");

	oldMask = 0123;
	umask(oldMask);

	/* Check all the possible values of umask. */
	for (i = 0000; i <= 0777; ++i) {
		if (oldMask != umask(i)) e(1);
		fd1 = open("open", O_CREAT, 0777);
		if (fd1 != 3) e(2);
		fd2 = creat("creat", 0777);
		if (fd2 != 4) e(3);
		if (mkdir("dir", 0777) != 0) e(4);
		if (mkfifo("fifo", 0777) != 0) e(5);

		if (umode("open") != i) e(6);
		if (umode("creat") != i) e(7);
		if (umode("dir") != i) e(8);
		if (umode("fifo") != i) e(9);

		if (close(fd1) != 0) e(10);
		if (close(fd2) != 0) e(11);
		unlink("open");
		unlink("creat");
		rmdir("dir");
		unlink("fifo");
		oldMask = i;
	}

	/* Check-reset msak */
	if (umask(0124) != 0777) e(12);
	
	/* Check if a umask of 0000 leaves the modes alone. */
	if (umask(0000) != 0124) e(13);

	for (i = 0000; i <= 0777; ++i) {
		fd1 = open("open", O_CREAT, i);
		if (fd1 != 3) e(14);
		fd2 = creat("creat", i);
		if (fd2 != 4) e(15);
		if (mkdir("dir", i) != 0) e(16);
		if (mkfifo("fifo", i) != 0) e(17);

		if (mode("open") != i) e(18);
		if (mode("creat") != i) e(19);
		if (mode("dir") != i) e(20);
		if (mode("fifo") != i) e(21);

		if (close(fd1) != 0) e(22);
		if (close(fd2) != 0) e(23);
		if (unlink("open") != 0) e(24);
		unlink("creat");
		rmdir("dir");
		unlink("fifo");
	}

	/* Check if umask survives a fork() */
	if (umask(0124) != 0000) e(25);

	switch (fork()) {
		case -1: 
			forkFailed();
		case 0: 
			mkdir("bar", 0777);
			exit(0); 
		default:
			if (wait(&status) == -1) e(26);
	}
	if (umode("bar") != 0124) e(27);
	rmdir("bar");

	/* Check if umask in child changes umask in parent. */
	switch (fork()) {
		case -1: 
			forkFailed();
		case 0: 
			switch (fork()) {
				case -1:
					forkFailed();
				case 0:
					if (umask(0432) != 0124) e(28);
					exit(0);
				default:
					if (wait(&status) == -1) e(29);
			}
			if (umask(0423) != 0124) e(30);
			exit(0);
		default:
			if (wait(&status) == -1) e(31);
	}
	if (umask(0342) != 0124) e(32);

	/* See if extra bits are ignored */
	if (umask(0xFFFF) != 0342) e(33);
	if (umask(0xFE00) != 0777) e(34);
	if (umask(01777) != 0000) e(35);
	if (umask(0022) != 0777) e(36);
}

void main(int argc, char **argv) {
	setup(argc, argv);

	test22a();

	quit();
}
