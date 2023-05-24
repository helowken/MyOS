#include <stdio.h>

size_t fread(void *ptr, size_t size, size_t nmemb, register FILE *stream) {
	register char *cp = ptr;
	register int c;
	size_t nDone = 0;
	register size_t s;

	if (size) {
		while (nDone < nmemb) {
			s = size;
			do {
				if ((c = getc(stream)) != EOF)
				  *cp++ = c;
				else
				  return nDone;
			} while (--s);
			++nDone;
		}
	}
	return nDone;
}
