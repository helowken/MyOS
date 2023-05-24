#include <lib.h>
#include <unistd.h>

int sync() {
	Message msg;

	return syscall(FS, SYNC, &msg);
}
