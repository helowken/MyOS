#ifndef _STDLIB_H
#define _STDLIB_H

#include <stddef.h>

#define EXIT_FAILURE	1		/* Standard error return using exit() */
#define EXIT_SUCCESS	0		/* Successful return using exit() */
#define RAND_MAX		32767	/* Largest value generated by rand() */

void abort();
int atoi(const char *nptr);
long atol(const char *nptr);
void *calloc(size_t nmemb, size_t size);
void exit(int status);
void free(void *op);
char *getenv(const char *name);
void *malloc(size_t size);
void *realloc(void *op, size_t size);
int rand(void);
void srand(unsigned int seed);
double strtod(const char *nptr, char **endptr);
long int strtol(const char *nptr, char **endptr, int base);
int system(const char *command);
unsigned long int strtoul(const char *nptr, char **endptr, int base);

#ifdef _POSIX_SOURCE
/* Open Group Base specifications Issue 6 */
int mkstemp(char *fmt);
char *initstate(unsigned seed, char *state, size_t size);
long random(void);
char *setstate(const char *state);
void srandom(unsigned seed);
#endif

#ifdef _MINIX
int putenv(const char *string);
#endif

#endif
