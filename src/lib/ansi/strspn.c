#include <string.h>

size_t strspn(const char *s, const char *accept) {
	register const char *s1, *s2;

	for (s1 = s; *s1; ++s1) {
		for (s2 = accept; *s2 && *s2 != *s1; ++s2) {
		}
		if (*s2 == '\0')
		  break;
	}
	return s1 - s;
}
