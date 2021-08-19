#include "code.h"
#include "string.h"

size_t strlen(const char *org) {
	register const char *s = org;

	while (*s++) {
	}
	return --s - org;
}
