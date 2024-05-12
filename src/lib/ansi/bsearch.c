#include <stdlib.h>

void *bsearch(register const void *key, register const void *base, 
			register size_t nmemb, register size_t size,
			int (*compar)(const void *, const void *)) {

	register const void *midPoint;
	register int cmp;

	while (nmemb > 0) {
		midPoint = (char *) base + size * (nmemb >> 1);
		if ((cmp = (*compar)(key, midPoint)) == 0)
		  return (void *) midPoint;
		if (cmp > 0) {
			base = (char *) midPoint + size;
			nmemb = (nmemb - 1) >> 1;
		} else
		  nmemb >>= 1;
	}
	return NULL;
}
