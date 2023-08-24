#include <lib.h>
#include <stddef.h>

void *memccpy(void *dst, const void *src, int c, size_t size) {
	register char *d;
	register const char *s;
	register size_t n;
	register int uc;

	if (size <= 0)
	  return (void *) NULL;

	s = (char *) src;
	d = (char *) dst;
	uc = (unsigned char) c;
	for (n = size; n > 0; --n) {
		if ((unsigned char) (*d++ = *s++) == uc)
		  return (void *) d;
	}
	return (void *) NULL;
}
