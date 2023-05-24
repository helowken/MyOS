#include <string.h>

char *strrchr(register const char *s, int c) {
	register const char *result = NULL;

	c = (char) c;

	do {
		if (c == *s)
		  result = s;
	} while (*s++ != 0);

	return (char *) result;
}
