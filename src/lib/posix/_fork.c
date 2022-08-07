#include "lib.h"
#include "unistd.h"

pid_t fork() {
	Message msg;

	return syscall(MM, FORK, &msg);
}
