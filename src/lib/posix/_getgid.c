#include "lib.h"
#include "unistd.h"

gid_t getgid() {
	Message msg;

	return (gid_t) syscall(MM, GETGID, &msg);
}
