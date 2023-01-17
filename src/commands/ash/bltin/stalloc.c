#include "../shell.h"

void error();
pointer malloc(size_t size);

pointer stackAlloc(size_t bytes) {
	register pointer p;

	if ((p = malloc(bytes)) == NULL)
	  error("Out of space");
	return p;
}
