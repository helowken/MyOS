#ifndef _STRING_H
#define _STRING_H

#include <stddef.h>

void *memchr(const void *s, int c, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);
void *memcpy(void *s1, const void *s2, size_t n);
void *memset(void *s, int c, size_t n);
void *memmove(void *dest, const void *src, size_t n);
char *strcat(char *dest, const char *src);
char *strncat(char *dest, const char *src, size_t n);
char *strchr(const char *s, int c);
int strncmp(const char *s1, const char *s2, size_t n);
int strcmp(const char *s1, const char *s2);
char *strcpy(char *ret, const char *s2);
char *strerror(int errnum);
size_t strlen(const char *s);
char *strncpy(char *ret, const char *s2, size_t n);
char *strrchr(const char *s, int c);
size_t strspn(const char *s, const char *accept);
char *strstr(const char *s, const char *wanted);
char *strtok(char *str, const char *delim);
char *strpbrk(const char *s, const char *accept);


#ifdef _POSIX_SOURCE
/* Open Group Base Specifications Issue 6 (not complete) */
char *strdup(const char *s);
#endif

#ifdef _MINIX
/* For backward compatibility. */
#define index(s, c)		strchr((s), (c))
#define rindex(s, c)	strrchr((s), (c))
void bzero(void *s, size_t n);
#endif

#endif
