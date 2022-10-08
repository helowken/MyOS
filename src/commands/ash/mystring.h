

char *strchr(const char *, int);
int strlen(const char *);	/* From C library */
char *strcpy(char *, const char *);		/* From C library */
int isPrefix(const char *, const char *);


#define scopy(s1, s2)	((void) strcpy(s2, s1))
