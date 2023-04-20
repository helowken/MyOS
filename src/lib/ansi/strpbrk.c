#include "string.h"

char *strpbrk(register const char *s, register const char *accept) {
	register const char *s1;

	while (*s) {
		for (s1 = accept; *s1 && *s1 != *s; ++s1) {
		}
		if (*s1)
		  return (char *) s;
		++s;
	}
	return (char *) NULL;
}
