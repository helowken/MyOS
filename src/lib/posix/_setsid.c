#include "lib.h"
#include "unistd.h"

pid_t setsid() {
	Message msg;

	return syscall(MM, SETSID, &msg);
}
