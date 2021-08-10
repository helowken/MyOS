#ifndef _STRING_H
#define _STRING_H

#include "stddef.h"

void* memcpy(void *s1, const void *s2, size_t n);
char* strcpy(char *ret, const char *s2);
char* strcat(char *ret, const char *s2);

#endif
