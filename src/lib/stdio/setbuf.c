#include <stdio.h>
#include "loc_incl.h"

void setbuf(FILE *stream, char *buf) {
	setvbuf(stream, buf, (buf ? _IOFBF : _IONBF), (size_t) BUFSIZ);
}
