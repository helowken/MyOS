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
	   do_memset.o \
	   do_sigsend.o \
	   do_sigreturn.o \
	   do_copy.o \
	   do_devio.o \
	   do_irqctl.o \
	   do_vdevio.o \
	   do_sdevio.o \
	   do_umap.o \
	   do_segctl.o \
	   do_newmap.o \
	   do_abort.o

all: $(LIB)

clean: 
	$(call cleanCommon)
	rm -f $(LIB)

$(LIB): clean $(OBJS)
	ar rcs $@ $(OBJS)

