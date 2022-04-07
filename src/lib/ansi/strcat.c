#include "code.h"
#include "string.h"

char *strcat(char *ret, register const char *s2) {
	register char *s1 = ret;
	
	while (*s1++ != '\0');
	--s1;
	while ((*s1++ = *s2++));
	return ret;
}
