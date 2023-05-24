#include <stdlib.h>

#define ALIGN(x)	(((x) + (sizeof(size_t) - 1)) & ~(sizeof(size_t) - 1))


void *calloc(size_t nmemb, size_t elSize) {
	register char *p;
	register size_t *q;
	size_t size = ALIGN(nmemb * elSize);

	p = malloc(size);
	if (p == NULL)
	  return NULL;
	q = (size_t *) (p + size);
	while ((char *) q > p) {
		*--q = 0;
	}
	return p;
}
