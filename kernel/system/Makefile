MY_HOME = ../..
include $(MY_HOME)/common.mk

LIB = ../libmysystem.a

OBJS = do_unused.o \
	   do_fork.o \
	   do_exec.o \
	   do_exit.o \
	   do_nice.o \
	   do_privctl.o

all: $(LIB)

clean: clean_objs
	rm -f $(LIB)

clean_objs:
	rm -f $(OBJS)

$(LIB): clean_objs $(OBJS)
	ar rcs $@ $(OBJS)

