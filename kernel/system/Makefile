MY_HOME = ../..
include $(MY_HOME)/common.mk

LIB = ../libmysystem.a

OBJS = do_unused.o \
	   do_fork.o \
	   do_exec.o \
	   do_exit.o \
	   do_nice.o \
	   do_privctl.o \
	   do_trace.o \
	   do_kill.o \
	   do_getksig.o \
	   do_endksig.o \
	   do_getinfo.o \
	   do_times.o \
	   do_setalarm.o \
	   do_memset.o

all: $(LIB)

clean: clean_objs
	rm -f $(LIB)

clean_objs:
	rm -f $(OBJS)

$(LIB): clean_objs $(OBJS)
	ar rcs $@ $(OBJS)

