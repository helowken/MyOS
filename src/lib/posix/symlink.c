#include <errno.h>
#include <unistd.h>

int symlink(const char *target, const char *linkpath) {
	errno = ENOSYS;
	return -1;
}
