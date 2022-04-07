#include "lib.h"
#include "unistd.h"

uid_t getuid() {
	Message msg;

	return (uid_t) syscall(MM, GETUID, &msg);
}
