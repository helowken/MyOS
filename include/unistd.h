#ifndef _UNISTD_H
#define _UNISTD_H

#ifndef _TYPES_H
#include "sys/types.h"
#endif

/* Values used for whence in lseek(fd, offset, whence). POSIX table 2-9. */
#define SEEK_SET		0	/* Offset is absolute */
#define SEEK_CUR		1	/* Offset is relative to current position */
#define SEEK_END		2	/* Offset is relative to end of file */


/* What system info to retrieve with sysGetInfo(). */
#define SI_KINFO		0	/* Get kernle info via PM. */
#define SI_PROC_ADDR	1	/* Address of process table */
#define SI_PROC_TAB		2	/* Copy of entire process table */

gid_t getegid();
uid_t geteuid();
gid_t getgid();
uid_t getuid();
pid_t getpid();
pid_t getppid();
pid_t getpgrp();
pid_t setsid();
int setuid(uid_t uid);
int setgid(gid_t gid);
int pause();



int brk(char *addr);
char *sbrk(int incr);
int getProcNum();
int findProc(char *procName, int *pNum);
ssize_t write(int fd, const void *buf, size_t count);
int close(int fd);

#endif
