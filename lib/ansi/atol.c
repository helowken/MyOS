#include "code.h"
#include "ctype.h"
#include "stdlib.h"

long atol(register const char *nptr) {
	return strtol(nptr, (char **) NULL, 10);
}
