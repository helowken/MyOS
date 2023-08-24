#include "util.h"

static jmp_buf env;
static char buf[512];
static char *tmpa;

/* Return address of local variable.
 * This way we can check that the stack is not polluted.
 */
static char *addr() {
	char a;
	long i;

	i = (long) &a;
	return (char *) i;
}

static void dummy(int i, char *p) {
}

static void level1() {
	register char *p;
	register int i;

	i = 1000;
	p = &buf[10];
	i = 200;
	p = &buf[20];
	dummy(i, p);
	longjmp(env, 2);
}

static void dolev() {
	register char *p;
	register int i;

	i = 010;
	p = &buf[3];
	*p = i;
	longjmp(env, 3);
}

static void level2() {
	register char *p;
	register int i;

	i = 0200;
	p = &buf[2];
	*p = i;
	dolev();
}

static void catch(int sig) {
	longjmp(env, 4);
}

static void hard() {
	register char *p;

	signal(SIGHUP, catch);
	for (p = buf; p <= &buf[511]; ++p) {
		*p = 025;
	}
	kill(getpid(), SIGHUP);
}

static void garbage() {
	register int i, j, k;
	register char *p, *q, *r;
	char *a;

	p = &buf[300];
	q = &buf[400];
	r = &buf[500];
	i = 10;
	j = 20;
	k = 30;

	switch (setjmp(env)) {
		case 0:
			a = addr();
			srand((unsigned) &a);
			longjmp(env, 1);
			break;
		case 1:
			if (i != 10) e(11);
			if (j != 20) e(12);
			if (k != 30) e(13);
			if (p != &buf[300]) e(14);
			if (q != &buf[400]) e(15);
			if (r != &buf[500]) e(16);
			tmpa = addr();
			if (a != tmpa) e(17);
			level1();
			break;
		case 2:
			if (i != 10) e(21);
			if (j != 20) e(22);
			if (k != 30) e(23);
			if (p != &buf[300]) e(24);
			if (q != &buf[400]) e(25);
			if (r != &buf[500]) e(26);
			tmpa = addr();
			if (a != tmpa) e(27);
			level2();
			break;
		case 3:
			if (i != 10) e(31);
			if (j != 20) e(32);
			if (k != 30) e(33);
			if (p != &buf[300]) e(34);
			if (q != &buf[400]) e(35);
			if (r != &buf[500]) e(36);
			tmpa = addr();
			if (a != tmpa) e(37);
			hard();
		case 4:
			if (i != 10) e(41);
			if (j != 20) e(42);
			if (k != 30) e(43);
			if (p != &buf[300]) e(44);
			if (q != &buf[400]) e(45);
			if (r != &buf[500]) e(46);
			tmpa = addr();
			if (a != tmpa) e(47);
			return;
		default:
			e(100);
	}
	e(200);
}

static void test9a() {
	register int p;

	subTest = 1;
	p = 200;
	garbage();
	if (p != 200) e(1);
}

static void test9b() {
	register int p, q;
	
	subTest = 2;
	p = 200;
	q = 300;
	garbage();
	if (p != 200) e(1);
	if (q != 300) e(2);
}

static void test9c() {
	register int p, q, r;
	
	subTest = 3;
	p = 200;
	q = 300;
	r = 400;
	garbage();
	if (p != 200) e(1);
	if (q != 300) e(2);
	if (r != 400) e(3);
}

static void test9d() {
	register char *p;

	subTest = 4;
	p = &buf[100];
	garbage();
	if (p != &buf[100]) e(1);
}

static void test9e() {
	register char *p, *q;

	subTest = 5;
	p = &buf[100];
	q = &buf[200];
	garbage();
	if (p != &buf[100]) e(1);
	if (q != &buf[200]) e(2);
}

static void test9f() {
	register char *p, *q, *r;

	subTest = 6;
	p = &buf[100];
	q = &buf[200];
	r = &buf[300];
	garbage();
	if (p != &buf[100]) e(1);
	if (q != &buf[200]) e(2);
	if (r != &buf[300]) e(3);
}

void main(int argc, char **argv) {
	int i, j;
	jmp_buf envm;

	setup(argc, argv);

	for (j = 0; j < 100; ++j) {
		if (m & 00001) test9a();
		if (m & 00002) test9b();
		if (m & 00004) test9c();
		if (m & 00010) test9d();
		if (m & 00020) test9e();
		if (m & 00040) test9f();
	}
	if (errCnt) 
	  quit();

	i = 1;
	if (setjmp(envm) == 0) {
		i = 2;
		longjmp(envm, 1);
	} else {
		if (i == 2) {
			/* Correct */
		} else if (i == 1) {
			printf("WARNING: The setjmp/longjmp of this machine restore register variables\n\
						to the value they had at the time of the setjmp\n");
		} else {
			printf("Aha, I just found one last error\n");
			exit(1);
		}
	}
	quit();
}
