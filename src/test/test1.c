#include "util.h"

#define SIG_NUM		10
#define MAX_ERRO	4
#define ITERATIONS	10

volatile int glov, gct;

static void parent() {
	int n;

	getpid();
	wait(&n);
}

static void child(int i) {
	getpid();
	exit(100 + i);
}

static void test1a() {
	int i, n, pid;

	subTest = 1;
	n = 4;
	for (i = 0; i < n; ++i) {
		if ((pid = fork())) {
			if (pid < 0) {
				fail("\nTest 1 fork failed\n");
			}
			parent();
		} else {
			child(i);
		}
	}
}

static void parent1(int childPid) {
	int n;

	for (n = 0; n < 5000; ++n) {
	}
	while (kill(childPid, SIG_NUM) < 0) {
	}
	wait(&n);
}

static void child1() {
	while (glov == 0) {
	}
	exit(gct);
}

static void func(int s) {
	++glov;
	++gct;
}

static void test1b() {
	int i, k;

	subTest = 2;
	for (i = 0; i < 4; ++i) {
		glov = 0;
		signal(SIG_NUM, func);
		if ((k = fork())) {
			if (k < 0) {
				fail("Test 1 fork failed\n");
			}
			parent1(k);
		} else {
			child1();
		}
	}
}

void main(int argc, char **argv) {
	int i;

	setup(argc, argv);
	for (i = 0; i < ITERATIONS; ++i) {
		if (m & 00001)
		  test1a();
		if (m & 00002)
		  test1b();
	}

	quit();
}
