MY_HOME = ../..
LIB_NAME = myc
NEED_16 = 1

OBJS_M32 = \
		memchr.o \
		memcmp.o \
		memcpy.o \
		memset.o \
		memmove.o \
		strcat.o \
		strncat.o \
		strcpy.o \
		strncpy.o \
		strlen.o \
		strcmp.o \
		strncmp.o \
		strchr.o \
		strstr.o \
		strrchr.o \
		strspn.o \
		strtol.o \
		strpbrk.o \
		strtok.o \
		chartab.o \
		atoi.o \
		atol.o \
		exit.o \
		abort.o \
		raise.o \
		signal.o \
		getenv.o \
		sigmisc.o \
		malloc.o \
		calloc.o \
		strerror.o \
		errlist.o \
		toupper.o \
		tolower.o \
		clock.o \
		difftime.o \
		mktime.o \
		misc.o \
		asctime.o \
		ctime.o \
		gmtime.o \
		localtime.o \
		strftime.o \
		tzset.o \
		assert.o \
		rand.o \
		system.o \
		strcspn.o \
		bsearch.o \
		qsort.o \
		abs.o \
		atexit.o \
		labs.o

		
OBJS_M16 = \
		$(DIR_16)/memcpy.o \
		$(DIR_16)/strcat.o \
		$(DIR_16)/strcpy.o \
		$(DIR_16)/strncpy.o \
		$(DIR_16)/strlen.o \
		$(DIR_16)/strncmp.o \
		$(DIR_16)/strcmp.o \
		$(DIR_16)/strchr.o \
		$(DIR_16)/memset.o 
	

all: build

include ../commonlib.mk

build: $(LIB_M16) $(LIB_M32)

clean: clean16 clean32
