#include "lib.h"
#include "unistd.h"

int umount(const char *name) {
	Message msg;

	_loadName(name, &msg);
	return syscall(FS, UMOUNT, &msg);
}
