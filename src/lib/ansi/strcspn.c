#include <string.h>

size_t strcspn(const char *s, const char *reject) {
	register const char *s1, *s2;

	for (s1 = s; *s1; ++s1) {
		for (s2 = reject; *s2 && *s2 != *s1; ++s2) {
		}
		if (*s2)
		  break;
	}
	return s1 - s;
}
