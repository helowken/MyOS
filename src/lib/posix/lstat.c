#include <sys/stat.h>

int lstat(const char *path, struct stat *sb) {
	return stat(path, sb);
}
