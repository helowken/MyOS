#ifndef _STRING_H
#define _STRING_H

#include "stddef.h"

void *memcpy(void *s1, const void *s2, size_t n);
char *strcpy(char *ret, const char *s2);
char *strncpy(char *ret, const char *s2, size_t n);
char *strcat(char *ret, const char *s2);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);
size_t strlen(const char *s);
char *strchr(const char *s, int c);
char *strrchr(const char *s, int c);
char *strstr(const char *s, const char *wanted);
void *memset(void *s, int c, size_t n);

#endif
