#ifndef _UNISTD_H
#define _UNISTD_H

#ifndef _TYPES_H
#include "sys/types.h"
#endif

/* Values used by access(). */
#define F_OK			0	/* Test if file exists */
#define X_OK			1	/* Test if file is executable */
#define W_OK			2	/* Test if file is writable */
#define R_OK			3	/* Test if file is readable */

/* Values used for whence in lseek(fd, offset, whence). POSIX table 2-9. */
#define SEEK_SET		0	/* Offset is absolute */
#define SEEK_CUR		1	/* Offset is relative to current position */
#define SEEK_END		2	/* Offset is relative to end of file */

#ifdef _MINIX
/* How to exit the system or stop a server process. */
#define RBT_HALT		0	
#define RBT_REBOOT		1
#define RBT_PANIC		2	/* A server panics */
#define RBT_MONITOR		3	/* Let the monitor do this */ 
#define RBT_RESET		4	/* Hard reset the system */
#endif

/* What system info to retrieve with sysGetInfo(). */
#define SI_KINFO		0	/* Get kernle info via PM. */
#define SI_PROC_ADDR	1	/* Address of process table */
#define SI_PROC_TAB		2	/* Copy of entire process table */

/* NULL must be defined in <unistd.h> according to POSIX. */
#ifndef NULL
#define NULL	((void *) 0)
#endif

void _exit(int status);
int access(const char *path, int mode);
int dup(int fd);
int execve(const char *path, char *const argv[], char *const envp[]);
pid_t fork();
gid_t getegid();
uid_t geteuid();
gid_t getgid();
uid_t getuid();
int isatty(int fd);
pid_t getpid();
pid_t getppid();
pid_t getpgrp();
pid_t setsid();
int setuid(uid_t uid);
int setgid(gid_t gid);
off_t lseek(int fd, off_t offset, int whence);
int pause();
ssize_t read(int fd, void *buf, size_t n);
int unlink(const char *path);
ssize_t write(int fd, const void *buf, size_t count);


#ifdef _MINIX
#ifndef _TYPE_H
#include "minix/type.h"
#endif

int brk(char *addr);
char *sbrk(int incr);
int getProcNum();
int findProc(char *procName, int *pNum);
int close(int fd);
int allocMem(phys_bytes size, phys_bytes *base);
#endif

#endif
