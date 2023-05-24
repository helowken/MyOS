#include <stdio.h>

size_t fwrite(const void *ptr, size_t size, size_t nmemb, 
			register FILE *stream) {
	register const unsigned char *cp = ptr;
	register size_t s;
	size_t nDone = 0;

	if (size) {
		while (nDone < nmemb) {
			s = size;
			do {
				if (putc((int) *cp, stream) == EOF)
				  return nDone;
				++cp;
			} while (--s);
			++nDone;
		}
	}
	return nDone;
}
