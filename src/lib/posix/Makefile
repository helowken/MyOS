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
		_fstat.o \
		_open.o \
		_dup.o \
		_fcntl.o \
		_fork.o \
		_execve.o \
		_access.o \
		_close.o \
		__exit.o \
		_read.o \
		_write.o \
		_lseek.o \
		_isatty.o \
		_tcgetattr.o \
		_ioctl.o \
		_kill.o


all: build

include ../commonlib.mk

build: $(LIB_M32)

clean: clean32
