MY_HOME = ../..
LIB_NAME = mysys

OBJS_M32 = \
		sys_getinfo.o \
		sys_setalarm.o \
		sys_getsig.o \
		sys_endsig.o \
		sys_sigsend.o \
		sys_sigreturn.o \
		sys_vircopy.o \
		sys_kill.o \
		sys_trace.o \
		sys_exit.o \
		sys_times.o \
		sys_nice.o \
		sys_out.o \
		sys_in.o \
		sys_irqctl.o \
		sys_umap.o \
		sys_vio.o \
		sys_sdevio.o \
		sys_segctl.o \
		sys_physcopy.o \
		sys_abort.o \
		sys_fork.o \
		sys_newmap.o \
		sys_memset.o \
		sys_exec.o \
		taskcall.o 
		   
	
all: build

include ../commonlib.mk

build: $(LIB_M32)

clean: clean32
