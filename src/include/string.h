#ifndef _STRING_H
#define _STRING_H

#include "stddef.h"

int memcmp(const void *s1, const void *s2, size_t n);
void *memcpy(void *s1, const void *s2, size_t n);
void *memset(void *s, int c, size_t n);
char *strcat(char *ret, const char *s2);
char *strchr(const char *s, int c);
int strncmp(const char *s1, const char *s2, size_t n);
int strcmp(const char *s1, const char *s2);
char *strcpy(char *ret, const char *s2);
char *strerror(int errnum);
char *strncpy(char *ret, const char *s2, size_t n);
size_t strlen(const char *s);
char *strrchr(const char *s, int c);
char *strstr(const char *s, const char *wanted);

#endif
