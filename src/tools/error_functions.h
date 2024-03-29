#ifndef ERROR_FUNCTIONS_H
#define ERROR_FUNCTIONS_H

void errMsg(const char *format, ...);

void printErrMsg(int errnum, const char *format, ...);

#ifdef __GNUC__
#ifdef OTHER_OS
#define NORETURN __attribute__ ((__noreturn__))
#else
#define NORETURN
#endif	/* OTHER_OS end */
#endif

void errExit(const char *format, ...) NORETURN;

void err_exit(const char *format, ...) NORETURN;

void errExitEN(int errnum, const char *format, ...) NORETURN;

void fatal(const char *format, ...) NORETURN;

void usageErr(const char *format, ...) NORETURN;

void cmdLineErr(const char *format, ...) NORETURN;

#endif
