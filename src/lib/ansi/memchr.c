#include <string.h>

void *memchr(const void *s, register int c, size_t n) {
	register const unsigned char *s1 = s;

	c = (unsigned char) c;
	if (n) {
		++n;
		while (--n > 0) {
			if (*s1++ != c)
			  continue;
			return (void *) --s1;
		}
	}
	return NULL;
}
