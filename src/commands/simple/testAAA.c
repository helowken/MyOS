#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

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

int main() {
	int fd;

	if ((fd = open("/dev/tty", O_RDWR)) == -1) {
		perror("open failed");
		return 1;
	}
	printf("fd: %d\n", fd); 
	close(fd);
	return 0;
}
