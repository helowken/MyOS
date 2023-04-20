#include "string.h"

void *memmove(void *dest, const void *src, size_t n) {
	register char *p1 = dest;
	register const char *p2 = src;

	if (n > 0) {
		if (p2 <= p1 && p2 + n > p1) {
			/* Overlap, copy backwards */
			p1 += n;
			p2 += n;
			++n;
			while (--n > 0) {
				*--p1 = *--p2;
			}
		} else {
			++n;
			while (--n > 0) {
				*p1++ = *p2++;
			}
		}
	}
	return dest;
}
