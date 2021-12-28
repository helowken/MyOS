#ifndef _UNISTD_H
#define _UNISTD_H

#ifndef _TYPES_H
#include "sys/types.h"
#endif

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

#endif
