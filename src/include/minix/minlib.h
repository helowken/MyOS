#ifndef _MINLIB
#define _MINLIB

/* Miscellaneous MINIX. */
void stdErr(const char *s);
void tell(const char *s);
void report(const char *prog, const char *s);
void fatal(const char *prog, const char *s);
void usage(const char *prog, const char *s);
int fsVersion(char *dev, char *prog);
int loadMTab(char *prog);
int rewriteMTab(char *prog);
int getMTabEntry(char *special, char *mountedOn, char *version, char *rwFlag);
int putMTabEntry(char *special, char *mountedOn, char *version, char *rwFlag);

#endif
