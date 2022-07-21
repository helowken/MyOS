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
		_open.o 


all: build

include ../commonlib.mk

build: $(LIB_M32)

clean: clean32
