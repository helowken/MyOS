#include "lib.h"
#include "unistd.h"

int unlink(const char *path) {
	Message msg;

	_loadName(path, &msg);
	return syscall(FS, UNLINK, &msg);
}
