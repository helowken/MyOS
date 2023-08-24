#include "util.h"

#define ITERATIONS	60

#define Fstat(a, b)		if (fstat((a), (b)) != 0) printf("Can't fstat %d\n", (a))
#define Time(t)			if (time((t)) == (time_t) -1) printf("Time error\n")



static void test8a() {
	int tube[2], t1[2], t2[2], t3[2];
	time_t time1, time2;
	char buf[128];
	struct stat st1, st2;
	int statLoc, flags;

	subTest = 1;

	/* Check if lowest fds are returned. */
	if (pipe(tube) != 0) e(1);
	if (tube[0] != 3 && tube[1] != 3) e(2);
	if (tube[0] != 4 && tube[1] != 4) e(3);
	if (tube[0] == tube[1]) e(4);
	if (pipe(t1) != 0) e(5);
	if (t1[0] != 5 && t1[1] != 5) e(6);
	if (t1[0] != 6 && t1[1] != 6) e(7);
	if (t1[0] == t1[1]) e(8);
	if (close(t1[0]) != 0) e(9);
	if (close(tube[0]) != 0) e(10);
	if (pipe(t2) != 0) e(11);
	if (t2[0] != tube[0] && t2[1] != tube[0]) e(12);
	if (t2[0] != t1[0] && t2[1] != t1[0]) e(13);
	if (t2[0] == t2[1]) e(14);
	if (pipe(t3) != 0) e(15);
	if (t3[0] != 7 && t3[1] != 7) e(16);
	if (t3[0] != 8 && t3[1] != 8) e(17);
	if (t3[0] == t3[1]) e(18);
	if (close(tube[1]) != 0) e(19);
	if (close(t1[1]) != 0) e(20);
	if (close(t2[0]) != 0) e(21);
	if (close(t2[1]) != 0) e(22);
	if (close(t3[0]) != 0) e(23);
	if (close(t3[1]) != 0) e(24);

	/* All time fields should be marked for update. */
	Time(&time1);
	if (pipe(tube) != 0) e(25);
	Fstat(tube[0], &st1);
	Fstat(tube[1], &st2);
	Time(&time2);
	if (st1.st_atime < time1) e(26);
	if (st1.st_ctime < time1) e(27);
	if (st1.st_mtime < time1) e(28);
	if (st1.st_atime > time2) e(29);
	if (st1.st_ctime > time2) e(30);
	if (st1.st_mtime > time2) e(31);

	if (st2.st_atime < time1) e(32);
	if (st2.st_ctime < time1) e(33);
	if (st2.st_mtime < time1) e(34);
	if (st2.st_atime > time2) e(35);
	if (st2.st_ctime > time2) e(36);
	if (st2.st_mtime > time2) e(37);

	/* Check the file characteristics. */
	if ((flags = fcntl(tube[0], F_GETFD)) != 0) e(38);
	if ((flags & FD_CLOEXEC) != 0) e(39);
	if ((flags = fcntl(tube[0], F_GETFL)) != 0) e(40);
	if ((flags & O_RDONLY) != O_RDONLY) e(41);
	if ((flags & O_NONBLOCK) != 0) e(42);
	if ((flags & O_RDWR) != 0) e(43);
	if ((flags & O_WRONLY) != 0) e(44);

	if ((flags = fcntl(tube[1], F_GETFD)) != 0) e(45);
	if ((flags & FD_CLOEXEC) != 0) e(46);
	if ((flags = fcntl(tube[1], F_GETFL)) == -1) e(47);
	if ((flags & O_WRONLY) != O_WRONLY) e(48);
	if ((flags & O_NONBLOCK) != 0) e(49);
	if ((flags & O_RDWR) != 0) e(50);
	if ((flags & O_RDONLY) != 0) e(51);

	/* Check if we can read and write. */
	switch (fork()) {
		case -1: 
			forkFailed();
		case 0: 
			alarm(20);
			if (close(tube[0]) != 0) e(52);
			if (write(tube[1], "Hello", 6) != 6) e(53);
			if (close(tube[1]) != 0) e(54);
			exit(0);
		default:
			if (read(tube[0], buf, sizeof(buf)) != 6) e(55);
			if (strncmp(buf, "Hello", 6) != 0) e(56);
			wait(&statLoc);
			if (statLoc != 0) e(57);	/* => WIFEXITED && WEXITSTATUS(statLoc) == 0 */
	}
	if (close(tube[0]) != 0) e(58);
	if (close(tube[1]) != 0) e(59);
}

static void test8b() {
	int tube[2], child2Parent[2], parent2Child[2];
	int i, nChild = 0, nOpen = 3, statLoc;
	int fd;
	int c;

	subTest = 2;

	/* Take all the pipes we can get. */
	while (nOpen < OPEN_MAX - 2) {
		if (pipe(tube) != 0) {
			/* We have not reached OPEN_MAX yet, so we have ENFILE. */
			if (errno != ENFILE) e(1);
			sleep(2);	/* Wait for others to (maybe) close files. */
			break;
		}
		nOpen += 2;
	}

	if (nOpen < OPEN_MAX - 2) {
		if (pipe(tube) != -1) e(2);
		switch (errno) {
			case EMFILE:	/* Error value is ok. */
				break;
			case ENFILE:	/* no process can open files any more. */
				switch (fork()) {
					case -1:
						forkFailed();
					case 0:
						alarm(20);
						if (open("/", O_RDONLY) != -1) e(3);
						if (errno != ENFILE) e(4);
						exit(0);
					default:
						wait(&statLoc);
						if (statLoc != 0) e(5);
				}
				break;
			default:
				e(6);	/* Wrong value for errno */
		}
	}

	/* Close all but stdin, out, err. */
	for (i = 3; i < OPEN_MAX; ++i) {
		close(i);
	}

	/* ENFILE test. Have children each grab OPEN_MAX fds. */
	if (pipe(child2Parent) != 0) e(7);
	if (pipe(parent2Child) != 0) e(8);
	while ((fd = open("/", O_RDONLY)) != -1) {
		close(fd);
		switch (fork()) {
			case -1:
				forkFailed();
			case 0:
				alarm(60);

				/* Grab all the fds. */
				while (pipe(tube) != -1) {
				}
				while (open("/", O_RDONLY) != -1) {
				}

				/* Signal parent OPEN_MAX fds gone. */
				if (write(child2Parent[1], "*", 1) != 1) e(9);

				/* Wait for parent before freeing all the fds. */
				if (read(parent2Child[0], &c, 1) != 1) e(10);
				exit(0);
			default:
				/* Wait for child to grab OPEN_MAX fds. */
				if (read(child2Parent[0], &c, 1) != 1) e(11);
				++nChild;
				break;
		}
	}

	if (pipe(tube) != -1) e(12);
	if (errno != ENFILE) e(13);

	/* Signal children to die and wait for it. */
	while (nChild-- > 0) {
		if (write(parent2Child[1], "*", 1) != 1) e(14);
		wait(&statLoc);
		if (statLoc != 0) e(15);
	}

	/* Close all but stdin, out, err. */
	for (i = 3; i < OPEN_MAX; ++i) {
		close(i);
	}
}

void main(int argc, char **argv) {
	int i;

	setup(argc, argv);

	for (i = 0; i < ITERATIONS; ++i) {
		if (m & 0001) test8a();
		if (m & 0002) test8b();
	}
	quit();
}
