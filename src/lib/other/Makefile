MY_HOME = ../..
LIB_NAME = myc
NEED_16 = 1

OBJS_M32 = \
		errno.o \
		syscall.o \
		settimeofday.o \
		getprocnr.o \
		findproc.o \
		allocmem.o \
		loadname.o \
		_svrctl.o \
		_brk.o \
		environ.o \
		fdopen.o \
		_devctl.o \
		funcs.o \
		_seekdir.o \
		telldir.o \
		fsversion.o \
		mtab.o \
		_reboot.o \
		getpwent.o \
		getgrent.o \
		getopt.o \
		bzero.o \
		itoa.o \
		ttyname.o \
		_getsysinfo.o \
		putenv.o \
		getttyent.o \
		crypt.o \
		getpass.o \
		ttyslot.o \
		getlogin.o \
		ctermid.o \
		sysconf.o \
		cuserid.o \
		memccpy.o \
		bcopy.o \
		bcmp.o \
		v8regexp \
		v8regsub 


OBJS_M16 = \
		$(DIR_16)/errno.o 


all: build

include ../commonlib.mk
CFLAGS += -I../../servers

build: $(LIB_M16) $(LIB_M32)

clean: clean16 clean32
