MY_HOME = ../..
LIB_NAME = myc
NEED_16 = 1

OBJS_M32 = \
		memcpy.o \
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
		atol.o 

		
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
