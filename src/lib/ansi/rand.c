#include "stdlib.h"

static unsigned long int next = 1;

int rand(void) {
	next = next * 1103515245 + 12345;
	return (unsigned int) (next / (2 * (RAND_MAX + 1L)) % (RAND_MAX + 1L));
}

void srand(unsigned int seed) {
	next = seed;
}
