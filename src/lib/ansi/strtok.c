#include "string.h"

char *strtok(register char *str, const char *delim) {
	register char *s1, *s2;
	static char *saveString;

	if (str == NULL) {
		str = saveString;
		if (str == NULL)
		  return (char *) NULL;
	}

	s1 = str + strspn(str, delim);
	if (*s1 == '\0') {
		saveString = NULL;
		return (char *) NULL;
	}

	s2 = strpbrk(s1, delim);
	if (s2 != NULL)
	  *s2++ = '\0';
	saveString = s2;
	return s1;
}
