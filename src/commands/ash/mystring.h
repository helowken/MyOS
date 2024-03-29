

void myBcopy(const pointer, pointer, int);
int prefix(const char *, const char *);
char *strchr(const char *, int);
int number(const char *);
int isNumber(const char *);
int strlen(const char *);	/* From C library */
char *strcpy(char *, const char *);		/* From C library */
int strcmp(const char *, const char *);	/* From C library */
int isPrefix(const char *, const char *);
char *strcat(char *, const char *);		/* From C library */
char *strerror(int);		/* From C library */

#define equal(s1, s2)	(strcmp(s1, s2) == 0)
#define scopy(s1, s2)	((void) strcpy(s2, s1))
#define bcopy(src, dst, n)	myBcopy((pointer) (src), (pointer) (dst), n)
