#include "util.h"

#define System(cmd)		if (system(cmd) != 0) printf("``%s'' failed\n", cmd)
#define Chdir(dir)		if (Chdir(dir) != 0) printf("Can't goto %s\n", dir)
#define Stat(a,b)		if (stat(a,b) != 0) printf("Can't stat %s\n", a)


char executable[1024];
char maxName[NAME_MAX + 1];
char maxPath[PATH_MAX];
char tooLongName[NAME_MAX + 2];
char tooLongPath[PATH_MAX + 1];


static void makeLongNames() {
	register int i;

	memset(maxName, 'a', NAME_MAX);
	maxName[NAME_MAX] = 0;
	for (i = 0; i < PATH_MAX - 1; ++i) {
		maxPath[i++] = '.';
		maxPath[i] = '/';
	}
	maxPath[PATH_MAX - 1] = 0;

	strcpy(tooLongName, maxName);
	strcpy(tooLongPath, maxPath);

	tooLongName[NAME_MAX] = 'a';
	tooLongName[NAME_MAX + 1] = 0;
	tooLongPath[PATH_MAX - 1] = '/';
	tooLongPath[PATH_MAX] = 0;
}

static void test20a() {
	subTest = 1;
	System("rm -rf ../DIR_20/*");
}

static void test20b() {
	subTest = 2;
	System("rm -rf ../DIR_20/*");
}

static void test20c() {
	subTest = 3;
	System("rm -rf ../DIR_20/*");
}

static int doCheck() {
/* This routine checks that fds 0 through 4, 7, and 8 are open and the rest
 * is closed. It also checks if we can lock the first 10 bytes on fd no. 3 
 * and 4. It should not be possible to lock fd no. 3, but it should be
 * possible to lock fd no. 4. See ``test20d()'' for usage of this routine.
 */
	int i;
	int rv = 0;
	struct flock fl;

	fl.l_whence = SEEK_SET;
	fl.l_start = 0;
	fl.l_len = 10;

	/* All std.. are open. */
	if (fcntl(0, F_GETFD) == -1) rv |= 0x11;
	if (fcntl(1, F_GETFD) == -1) rv |= 0x11;
	if (fcntl(2, F_GETFD) == -1) rv |= 0x11;

	/* Fd no. 3, 4, 7 and 8 are open. */
	if (fcntl(3, F_GETFD) == -1) rv |= 0x12;
	if (fcntl(4, F_GETFD) == -1) rv |= 0x12;
	if (fcntl(7, F_GETFD) == -1) rv |= 0x12;
	if (fcntl(8, F_GETFD) == -1) rv |= 0x12;

	/* Fd no. 5, 6, and 9 through OPEN_MAX are closed. */
	if (fcntl(5, F_GETFD) != -1) rv |= 0x14;
	if (fcntl(6, F_GETFD) != -1) rv |= 0x14;
	for (i = 9; i < OPEN_MAX; ++i) {
		if (fcntl(i, F_GETFD) != -1) rv |= 0x18;
	}

	/* Fd no. 4 is not locked. */
	fl.l_type = F_WRLCK;
	if (fcntl(4, F_SETLK, &fl) == -1) rv |= 0x41;
	if (fcntl(4, F_GETLK, &fl) == -1) rv |= 0x42;

	/* Fd no. 8 is locked after the fork, it is ours. */
	fl.l_type = F_WRLCK;
	if (fcntl(8, F_SETLK, &fl) == -1) rv |= 0x41;
	if (fcntl(8, F_GETLK, &fl) == -1) rv |= 0x42;

	return rv;
}

static void test20d() {
/* Open fds 3, 4, 5 and 6. Set FD_CLOEXEC on 5 and 6. Exclusively lock the
 * first 10 bytes of fd no. 3. Shared lock fd no. 7. Lock fd no. 8 after
 * the fork. Do a ``exec()'' call with a funny argv[0] and check the return
 * value.
 */
	int fd3, fd4, fd5, fd6, fd7, fd8;
	int status;
	int doCheckRetVal;
	char *argv[2];
	struct flock fl;

	subTest = 4;

	argv[0] = "DO CHECK";
	argv[1] = (char *) NULL;

	fl.l_whence = SEEK_SET;
	fl.l_start = 0;
	fl.l_len = 10;

	/* Make dummy files and open them. */
	System("echo 'Great Balls of Fire!' > file3");
	System("echo 'Great Balls of Fire!' > file4");
	System("echo 'Great Balls of Fire!' > file7");
	System("echo 'Great Balls of Fire!' > file8");
	System("echo 'Great Balls of Fire!' > file");
	if ((fd3 = open("file3", O_RDWR)) != 3) e(1);
	if ((fd4 = open("file4", O_RDWR)) != 4) e(2);
	if ((fd5 = open("file", O_RDWR)) != 5) e(3);
	if ((fd6 = open("file", O_RDWR)) != 6) e(4);
	if ((fd7 = open("file7", O_RDWR)) != 7) e(5);
	if ((fd8 = open("file8", O_RDWR)) != 8) e(6);

	/* Set FD_CLOEXEC flags on fd5 and fd6. */
	if (fcntl(fd5, F_SETFD, FD_CLOEXEC) == -1) e(7);
	if (fcntl(fd6, F_SETFD, FD_CLOEXEC) == -1) e(8);

	/* Lock the first 10 bytes from fd3 (for writing). */
	fl.l_type = F_WRLCK;
	if (fcntl(fd3, F_SETLK, &fl) == -1) e(9);

	/* Lock (for reading) fd7. */
	fl.l_type = F_RDLCK;
	if (fcntl(fd7, F_SETLK, &fl) == -1) e(10);

	switch (fork()) {
		case -1: forkFailed();
		case 0: {
			alarm(20);

			/* Lock fd8. */
			fl.l_type = F_WRLCK;
			if (fcntl(fd8, F_SETLK, &fl) == -1) e(11);

			/* Check the lock on fd3 and fd7. */
			fl.l_type = F_WRLCK;
			if (fcntl(fd3, F_GETLK, &fl) == -1) e(12);
			if (fl.l_type != F_WRLCK) e(13);
			if (fl.l_pid != getppid()) e(14);
			fl.l_type = F_WRLCK;
			if (fcntl(fd7, F_GETLK, &fl) == -1) e(15);
			if (fl.l_type != F_RDLCK) e(16);
			if (fl.l_pid != getppid()) e(17);
	
			/* Check FD_CLOEXEC flags. */
			if ((fcntl(fd3, F_GETFD) & FD_CLOEXEC) != 0) e(18);
			if ((fcntl(fd4, F_GETFD) & FD_CLOEXEC) != 0) e(19);
			if ((fcntl(fd5, F_GETFD) & FD_CLOEXEC) != FD_CLOEXEC) e(20);
			if ((fcntl(fd6, F_GETFD) & FD_CLOEXEC) != FD_CLOEXEC) e(21);
			if ((fcntl(fd7, F_GETFD) & FD_CLOEXEC) != 0) e(22);
			if ((fcntl(fd8, F_GETFD) & FD_CLOEXEC) != 0) e(23);

			execlp(executable + 3, "DO CHECK", (char *) NULL);
			execlp(executable, "DO CHECK", (char *) NULL);
			printf("Can't exec %s or %s\n", executable + 3, executable);
			exit(0);
		}
		default: {
			wait(&status);
			if (WIFSIGNALED(status) ) e(24);	/* Alarm? */
			if (WIFEXITED(status) == 0)	{
				errCnt = 10000;
				quit();
			}
		}
	}

	/* Check the return value of doCheck(). */
	doCheckRetVal = WEXITSTATUS(status);
	if ((doCheckRetVal & 0x11) == 0x11) e(25); 
	if ((doCheckRetVal & 0x12) == 0x11) e(26); 
	if ((doCheckRetVal & 0x14) == 0x11) e(27); 
	if ((doCheckRetVal & 0x18) == 0x11) e(28); 
	if ((doCheckRetVal & 0x21) == 0x11) e(29); 
	if ((doCheckRetVal & 0x22) == 0x11) e(30); 
	if ((doCheckRetVal & 0x24) == 0x11) e(31); 
	if ((doCheckRetVal & 0x28) == 0x11) e(32); 
	if ((doCheckRetVal & 0x41) == 0x11) e(33); 
	if ((doCheckRetVal & 0x42) == 0x11) e(34); 
	if ((doCheckRetVal & 0x44) == 0x11) e(35); 
	if ((doCheckRetVal & 0x48) == 0x11) e(36); 
	if ((doCheckRetVal & 0x81) == 0x11) e(37); 
	if ((doCheckRetVal & 0x82) == 0x11) e(38); 
	if ((doCheckRetVal & 0x84) == 0x11) e(39); 
	if ((doCheckRetVal & 0x88) == 0x11) e(40); 

	switch (fork()) {
		case -1: forkFailed();
		case 0: {
			alarm(20);

			/* Lock fd8. */
			fl.l_type = F_WRLCK;
			if (fcntl(fd8, F_SETLK, &fl) == -1) e(41);

			execvp(executable + 3, argv);
			execvp(executable, argv);
			printf("Can't exec %s or %s\n", executable + 3, executable);
			exit(0);
		}
		default:
			wait(&status);
			if (WIFSIGNALED(status)) e(48);
	}

	doCheckRetVal = WEXITSTATUS(status);
	if ((doCheckRetVal & 0x11) == 0x11) e(49); 
	if ((doCheckRetVal & 0x12) == 0x11) e(50); 
	if ((doCheckRetVal & 0x14) == 0x11) e(51); 
	if ((doCheckRetVal & 0x18) == 0x11) e(52); 
	if ((doCheckRetVal & 0x21) == 0x11) e(53); 
	if ((doCheckRetVal & 0x22) == 0x11) e(54); 
	if ((doCheckRetVal & 0x24) == 0x11) e(55); 
	if ((doCheckRetVal & 0x28) == 0x11) e(56); 
	if ((doCheckRetVal & 0x41) == 0x11) e(57); 
	if ((doCheckRetVal & 0x42) == 0x11) e(58); 
	if ((doCheckRetVal & 0x44) == 0x11) e(59); 
	if ((doCheckRetVal & 0x48) == 0x11) e(60); 
	if ((doCheckRetVal & 0x81) == 0x11) e(61); 
	if ((doCheckRetVal & 0x82) == 0x11) e(62); 
	if ((doCheckRetVal & 0x84) == 0x11) e(63); 
	if ((doCheckRetVal & 0x88) == 0x11) e(64); 

	fl.l_type = F_UNLCK;
	if (fcntl(fd3, F_SETLK, &fl) == -1) e(65);
	if (fcntl(fd7, F_SETLK, &fl) == -1) e(66);

	if (close(fd3) != 0) e(67);
	if (close(fd4) != 0) e(68);
	if (close(fd5) != 0) e(69);
	if (close(fd6) != 0) e(70);
	if (close(fd7) != 0) e(71);
	if (close(fd8) != 0) e(72);

	System("rm -f ../DIR_20/*\n");
}

void main(int argc, char **argv) {
	int i;

	/* If we have to check things, call doCheck(). */
	if (strcmp(argv[0], "DO CHECK") == 0)
	  exit(doCheck());

	/* Get the path of the executable. */
	strcpy(executable, "../");
	strcat(executable, argv[0]);

	setup(argc, argv);

	makeLongNames();

	for (i = 0; i < 10; ++i) {
		test20a();
		test20b();
		test20c();
		test20d();
	}
	quit();
}
