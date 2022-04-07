#include "code.h"
#include "ctype.h"
#include "stdlib.h"

int atoi(register const char *nptr) {
	return strtol(nptr, (char **) NULL, 10);
}
