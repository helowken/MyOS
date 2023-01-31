#ifndef _UNISTD_H
#define _UNISTD_H

#ifndef _TYPES_H
#include "sys/types.h"
#endif

/* Values used by access(void). */
#define F_OK			0	/* Test if file exists */
#define X_OK			1	/* Test if file is executable */
#define W_OK			2	/* Test if file is writable */
#define R_OK			3	/* Test if file is readable */

/* Values used for whence in lseek(fd, offset, whence). POSIX table 2-9. */
#define SEEK_SET		0	/* Offset is absolute */
#define SEEK_CUR		1	/* Offset is relative to current position */
#define SEEK_END		2	/* Offset is relative to end of file */

/* These three definitions are required by POSIX */
#define STDIN_FILENO	0	/* File descriptor for stdin */
#define STDOUT_FILENO	1	/* File descriptor for stdout */
#define STDERR_FILENO	2	/* File descriptor for stderr */

#ifdef _MINIX
/* How to exit the system or stop a server process. */
#define RBT_HALT		0	
#define RBT_REBOOT		1
#define RBT_PANIC		2	/* A server panics */
#define RBT_MONITOR		3	/* Let the monitor do this */ 
#define RBT_RESET		4	/* Hard reset the system */
#endif

/* What system info to retrieve with sysGetInfo(void). */
#define SI_KINFO		0	/* Get kernle info via PM. */
#define SI_PROC_ADDR	1	/* Address of process table */
#define SI_PROC_TAB		2	/* Copy of entire process table */

/* NULL must be defined in <unistd.h> according to POSIX. */
#ifndef NULL
#define NULL	((void *) 0)
#endif

void _exit(int status);
int access(const char *path, int mode);
unsigned int alarm(unsigned int seconds);
int chdir(const char *path);
int fchdir(int fd);
int chown(const char *path, uid_t owner, gid_t group);
int close(int fd);
int dup(int fd);
int dup2(int fd, int fd2);
int execl(const char *path, const char *arg, ...);
int execle(const char *path, const char *arg, ...);
int execlp(const char *file, const char *arg, ...);
int execv(const char *path, char * const argv[]);
int execve(const char *path, char * const argv[], char * const envp[]);
int execvp(const char *file, char * const argv[]);
pid_t fork(void);
long fpathconf(int fd, int name);
char *getcwd(char *buf, size_t size);
gid_t getegid(void);
uid_t geteuid(void);
gid_t getgid(void);
int getgroups(int size, gid_t list[]);
char *getlogin(void);
pid_t getpgrp(void);
pid_t getpid(void);
pid_t getppid(void);
uid_t getuid(void);
int isatty(int fd);
int link(const char *oldpath, const char *newpath);
off_t lseek(int fd, off_t offset, int whence);
long pathconf(const char *path, int name);
int pause(void);
int pipe(int fds[2]);
ssize_t read(int fd, void *buf, size_t n);
int rmdir(const char *path);
int setgid(gid_t gid);
int setpgid(pid_t pid, pid_t pgid);
pid_t setsid(void);
int setuid(uid_t uid);
unsigned int sleep(unsigned seconds);
long sysconf(int name);
int unlink(const char *path);
ssize_t write(int fd, const void *buf, size_t count);

/* Open Group Base Specifications Issue 6 (not complete) */
int symlink(const char *target, const char *linkpath);
int readlink(const char *path, char *buf, size_t bufsiz);
int getopt(int argc, char * const argv[], const char *optstring);
extern char *optarg;
extern int optind, opterr, optopt;
int usleep(useconds_t useconds);


#ifdef _MINIX
#ifndef _TYPE_H
#include "minix/type.h"
#endif

int brk(char *addr);
int chroot(const char *path);
int mknod(const char *path, mode_t mode, dev_t dev);
int mknod4(const char *path, mode_t mode, dev_t dev, long size);
char *mktemp(char *template);
int mount(char *special, char *name, int flag);
char *sbrk(int incr);
int umount(const char *name);
int reboot(int how, ...);
int getProcNum(void);
int findProc(char *procName, int *pNum);
int allocMem(phys_bytes size, phys_bytes *base);
#define DEV_MAP		1
#define DEV_UNMAP	2
#define mapDriver(driver, device, style) \
			devCtl(DEV_MAP, driver, device, style)
#define unmapDriver(device)	\
			devCtl(DEV_UNMAP, 0, device, 0)
int devCtl(int ctlReq, int driver, int device, int style);

#endif

#endif
