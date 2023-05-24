#include <stdio.h>
#include "loc_incl.h"

void rewind(FILE *stream) {
	fseek(stream, 0L, SEEK_SET);
	clearerr(stream);
}
