#include <stdio.h>

char *fgets(char *s, register int size, register FILE *stream) {
	register int ch;
	register char *ptr;

	ptr = s;
	while (--size > 0 && (ch = getc(stream)) != EOF) {
		*ptr++ = ch;
		if (ch == '\n')
		  break;
	}
	if (ch == EOF) {
		if (feof(stream)) {
			if (ptr == s)
			  return NULL;
		} else {
			return NULL;
		}
	}
	*ptr = '\0';
	return s;
}
