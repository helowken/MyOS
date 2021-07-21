#include "code.h"
#include "string.h"

void *
memcpy(void *s1, const void *s2, register size_t n) {
	register char *p1 = s1;
	register const char *p2 = s2;
	
	if (n) {
		++n;
		while (--n > 0) {
			*p1++ = *p2++;
		}
	}
	return s1;
}
