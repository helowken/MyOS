#ifndef _STDLIB_H
#define _STDLIB_H

#include "stddef.h"

void *malloc(size_t size);
void free(void *op);
void *realloc(void *op, size_t size);

#endif
