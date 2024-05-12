#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

jmp_buf env;

void testJmp() {
	int i;

	switch (setjmp(env)) {
		case 0:
			printf("flags: %d\n", env->__flags);
			printf("mask: 0x%x\n", env->__mask);
			for (i = 0; i < 16; ++i) {
				printf("%p\n", env->__regs[i]);
			}
			break;
		case 1:
			printf("=== long jump with 1\n");
			longjmp(env, 2);
		case 2:
			printf("=== long jump with 2\n");
			longjmp(env, 3);
		case 3:
			printf("=== long jump with 3\n");
			return;
	}

	printf("============\n");
	longjmp(env, 1); 
}

int cmp(const void *v1, const void *v2) {
	int n1 = *((int *) v1);
	int n2 = *((int *) v2);
	printf("-- n1: %d, n2: %d\n", n1, n2);
	return n1 - n2;
}

void testBSearch(int n) {
	int ns[] = { 1, 2, 3 };
	int *r;	

	r = bsearch(&n, ns, 3, sizeof(int), cmp);
	if (r == NULL) 
	  printf("== not found\n");
	else
	  printf("== %d\n", *r);
}

static void print(char *msg, int *base, int count) {
	int i;

	printf("%s", msg);
	for (i = 0; i < count; ++i) {
		if (i > 0)
		  printf(", ");
		printf("%d", *base++);
	}
	printf("\n");
}

static int cmpf(const void *v1, const void *v2) {
	return (*(int *) v1) - (*(int *) v2);
}

void testQsort(int argc, char **argv) {
	int i;
	int *base, *p, count;

	if (argc < 2) {
		fprintf(stderr, "usage: %s, num [[, num] ...]\n");
		exit(1);
	}

	count = argc - 1;
	base = (int *) calloc(count, sizeof(*base));
	if (base == NULL) {
		perror("calloc");
		exit(1);
	}

	p = base;
	for (i = 1; i <= count; ++i) {
		*p++ = atoi(argv[i]);		
	}
	print("before sort: ", base, count);
	qsort(base, count, sizeof(*base), cmpf);
	print(" after sort: ", base, count);
}

static void printAbs(int i) {
	printf("%d, abs: %d\n", i, abs(i));
}

static void print_labs(long i) {
	printf("%d, abs: %d\n", i, labs(i));
}

void testAbs() {
	printAbs(5);
	printAbs(0);
	printAbs(-5);

	print_labs(7);
	print_labs(0);
	print_labs(-7);
}

static void onExit() {
	printf("hello 111\n");
}

static void onExit2() {
	printf("hello 222\n");
}

void testAtexit() {
	atexit(onExit);
	atexit(onExit2);
	exit(0);
}

int main(int argc, char **argv) {
	testQsort(argc, argv);
	return 0;
}
