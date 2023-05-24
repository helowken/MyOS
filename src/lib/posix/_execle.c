#define	nil	0

#include <unistd.h>
#include <stdarg.h>

int execle(const char *path, const char *arg, ...) {
/* execle("/bin/sh", "sh", "-c", "example", (char *) 0, my_env_array); */
	char * const * envp;
	va_list ap;
	
	va_start(ap, arg);

	/* Find the end of the argument array. */
	if (arg != nil) {
		while (va_arg(ap, const char *) != nil) {
		}
	}
	envp = va_arg(ap, char * const *);
	va_end(ap);

	return execve(path, (char * const *) &arg, envp);
}
