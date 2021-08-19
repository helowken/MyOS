#include "code.h"
#include "string.h"

int strcmp(register const char *s1, register const char *s2) {
	while (*s1 == *s2++) {
		if (*s1++ == '\0')
		  return 0;
	}
	if (*s1 == '\0')
	  return -1;
	if (*--s2 == '\0')
	  return 1;
	return (unsigned char) *s1 - (unsigned char) *s2;
}
