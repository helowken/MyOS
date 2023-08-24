#include <lib.h>
#include <unistd.h>

int chroot(const char *path) {
	Message msg;

	_loadName(path, &msg);
	return syscall(FS, CHROOT, &msg);
}
