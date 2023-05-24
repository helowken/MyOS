#include <stdio.h>

int unlink(const char *path);

int remove(const char *path) {
	return unlink(path);
}
