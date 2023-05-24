#include <string.h>

char *strerror(register int errnum) {
	extern const char *_sys_errlist[];
	extern const int _sys_nerr;

	if (errnum < 0 || errnum >= _sys_nerr)
	  return "unknown error";
	return (char *) _sys_errlist[errnum];
}
