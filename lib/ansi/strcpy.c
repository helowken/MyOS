#include "code.h"
#include "string.h"

char *strcpy(char *ret, const char *s2) {
	char *s1 = ret;
	while ((*s1++ = *s2++));
	return ret;
}

