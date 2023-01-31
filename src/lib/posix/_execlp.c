#include "unistd.h"

int execlp(const char *file, const char *arg, ...) {
/* execlp("sh", "sh", "-c", "example", (char *) 0); */
	return execvp(file, (char * const *) &arg);
}
