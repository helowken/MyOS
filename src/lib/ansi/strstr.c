#include "code.h"
#include "string.h"

char *
strstr(register const char *s, register const char *wanted) {
	register const size_t len = strlen(wanted);
	
	if (len == 0)
	  return (char *) s;
	while (*s != *wanted || strncmp(s, wanted, len)) {
		if (*s++ == '\0')
		  return (char *) NULL;
	}
	return (char *) s;
}
