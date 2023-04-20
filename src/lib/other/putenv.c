#include "stdlib.h"
#include "string.h"

#define ENTRY_INC	10
#define rounded(x)	(((x / ENTRY_INC) + 1) * ENTRY_INC)

extern const char ***_penviron;

int putenv(const char *string) {
	register const char **v = *_penviron;
	register char *r;
	static int size = 0;
	/* When size != 0, it contains the number of entries in the
	 * table (including the final NULL pointer). This means that the
	 * last non-null entry is environ[size - 2].
	 */

	if (! string)
	  return 0;
	if (*_penviron == NULL)
	  return 1;
	if ((r = strchr(string, '='))) {
		register const char *p, *q;

		*r = '\0';
		if (v != NULL) {
			while ((p = *v) != NULL) {
				q = string;
				while (*q && (*q++ == *p++)) {
				}
				if (*q || *p != '=') {
					++v;
				} else {
					/* The name was already in the environment. */
					*r = '=';
					*v = string;
					return 0;
				}
			}
		}
		*r = '=';
		v = *_penviron;
	}

	if (! size) {
		register const char **p;
		register int i = 0;

		if (v) {
			do {
				++i;
			} while (*v++);
		}
		if (! (v = malloc(rounded(i) * sizeof(char **))))
		  return 1;
		size = i;
		p = *_penviron;
		*_penviron = v;
		while ((*v++ = *p++)) {	/* Copy the environment */
		}
		v = *_penviron;
	} else if (! (size % ENTRY_INC)) {
		if (! (v = realloc(*_penviron, rounded(size) * sizeof(char **))))
		  return 1;
		*_penviron = v;
	}
	v[size - 1] = string;
	v[size] = NULL;
	++size;
	return 0;
}
