#include "assert.h"
#include "stdio.h"
#include "stdlib.h"

void __bad_assertion(const char *msg) {
	fputs(msg, stderr);
	abort();
}
