#include "stdio.h"
#include "stdlib.h"

char *commandName;

void error(char *msg, ...) {
	fprintf(stderr, "%s: %s\n", commandName, msg);
	exit(2);
}
