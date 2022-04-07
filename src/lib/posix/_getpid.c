#include "lib.h"
#include "unistd.h"

pid_t getpid() {
	Message msg;

	return (pid_t) syscall(MM, GETPID, &msg);
}
