#include <string.h>

char *strncat(char *dest, register const char *src, size_t n) {
	register char *s1 = dest;

	if (n > 0) {
		while (*s1++) {
		}
		--s1;
		while ((*s1++ = *src++)) {
			if (--n > 0)
			  continue;
			*s1 = '\0';
			break;
		}
	}
	return dest;
}
