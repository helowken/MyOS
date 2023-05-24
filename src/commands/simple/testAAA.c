#include "setjmp.h"
#include "signal.h"
#include <stdio.h>

jmp_buf env;

int main() {
	sigset_t set;
	int i;

	sigemptyset(&set);
	sigaddset(&set, SIGUSR1);
	sigaddset(&set, SIGALRM);
	sigprocmask(SIG_SETMASK, &set, NULL);

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
			return 0;
	}

	printf("============\n");
	longjmp(env, 1); 

	return 0;
}
