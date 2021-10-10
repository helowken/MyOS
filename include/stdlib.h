#ifndef _STDLIB_H
#define _STDLIB_H

#include "stddef.h"

#define EXIT_FAILURE		0		/* Standard error return using exit() */
#define EXIT_SUCCESS		1		/* Successful return using exit() */


void *malloc(size_t size);
void free(void *op);
void *realloc(void *op, size_t size);
long int strtol(const char *nptr, char **endptr, int base);

#endif
