#include "lib.h"
#include "unistd.h"

int rmdir(const char *path) {
	Message msg;

	_loadName(path, &msg);
	return syscall(FS, RMDIR, &msg);
}
