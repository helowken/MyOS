LIB_NAME = myc
OTHER = ../other
I386 = ../i386
MISC = $(I386)/misc
RTS = $(I386)/rts
INT64 = $(I386)/int64
POSIX = ../posix

OBJS_M32 = memcpy.o \
		   strcat.o \
		   strcpy.o \
		   strncpy.o \
		   strlen.o \
		   strcmp.o \
		   strncmp.o \
		   strchr.o \
		   strstr.o \
		   memset.o \
		   chartab.o \
		   strtol.o \
		   atoi.o \
		   atol.o \
		   $(OTHER)/errno.o \
		   $(OTHER)/syscall.o \
		   $(OTHER)/settimeofday.o \
		   $(OTHER)/_getprocnr.o \
		   $(OTHER)/_findproc.o \
		   $(MISC)/io_inb.o \
		   $(MISC)/io_inw.o \
		   $(MISC)/io_inl.o \
		   $(MISC)/io_outb.o \
		   $(MISC)/io_outw.o \
		   $(MISC)/io_outl.o \
		   $(MISC)/io_intr.o \
		   $(RTS)/_ipc.o \
		   $(RTS)/_sigreturn.o \
		   $(POSIX)/sigset.o \
		   $(POSIX)/_sigaction.o \
		   $(POSIX)/_sigreturn.o \
		   $(POSIX)/_sigprocmask.o \
		   $(POSIX)/_sigsuspend.o \
		   $(POSIX)/_sigpending.o \
		   $(POSIX)/_times.o \
		   $(POSIX)/_stime.o \
		   $(POSIX)/_time.o \
		   $(POSIX)/gettimeofday.o \
		   $(POSIX)/priority.o \
		   $(POSIX)/_getegid.o \
		   $(POSIX)/_geteuid.o \
		   $(POSIX)/_getgid.o \
		   $(POSIX)/_getuid.o \
		   $(POSIX)/_getpid.o \
		   $(POSIX)/_getppid.o \
		   $(POSIX)/_getpgrp.o \
		   $(POSIX)/_setsid.o \
		   $(POSIX)/_waitpid.o \
		   $(POSIX)/_wait.o \
		   $(INT64)/mul64u.o \
		   $(INT64)/cmp64.o \
		   $(INT64)/div64u.o \
		   $(INT64)/cv64u.o \
		   $(INT64)/add64u.o 

		
OBJS_M16 = memcpy.o \
		   strcat.o \
		   strcpy.o \
		   strncpy.o \
		   strlen.o \
		   strcmp.o \
		   strchr.o \
		   memset.o \
		   $(OTHER)/errno.o
	

include ../commonlib.mk

