#include "code.h"
#include "string.h"

char *strchr(register const char *s, register int c) {
	c = (char) c;

	while (c != *s) {
		if (*s++ == '\0')
		  return NULL;
	}
	return (char *) s;
}
