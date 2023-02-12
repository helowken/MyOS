MY_HOME = ../..
LIB_NAME = myc

OBJS_M32 = \
		sigset.o \
		_sigaction.o \
		_sigreturn.o \
		_sigprocmask.o \
		_sigsuspend.o \
		_sigpending.o \
		_times.o \
		_stime.o \
		_time.o \
		gettimeofday.o \
		priority.o \
		_getegid.o \
		_geteuid.o \
		_getgid.o \
		_getuid.o \
		_getpid.o \
		_getppid.o \
		_getpgrp.o \
		_setsid.o \
		_waitpid.o \
		_wait.o \
		_stat.o \
		_fstat.o \
		lstat.o \
		_open.o \
		_dup.o \
		_fcntl.o \
		_fork.o \
		_execl.o \
		_execle.o \
		_execlp.o \
		_execv.o \
		_execve.o \
		_execvp.o \
		_access.o \
		_close.o \
		__exit.o \
		_read.o \
		_write.o \
		_lseek.o \
		_isatty.o \
		_tcgetattr.o \
		_ioctl.o \
		_kill.o \
		_umask.o \
		_creat.o \
		_unlink.o \
		_rename.o \
		_pipe.o \
		_sleep.o \
		_alarm.o \
		_dup2.o \
		_chdir.o \
		_opendir.o \
		_closedir.o \
		_readdir.o \
		_rewinddir.o \
		_mount.o \
		_umount.o \
		_mkdir.o \
		_rmdir.o \
		_chown.o \
		_chmod.o \
		_utime.o \
		readlink.o \
		_getcwd.o \
		_link.o \
		symlink.o \
		_mknod.o


all: build

include ../commonlib.mk

build: $(LIB_M32)

clean: clean32
