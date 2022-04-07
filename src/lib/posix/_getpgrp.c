#include "lib.h"
#include "unistd.h"

pid_t getpgrp() {
	Message msg;

	return syscall(MM, GETPGRP, &msg);
}
