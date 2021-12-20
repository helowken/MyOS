LIB_NAME = myc
OTHER = ../other
I386 = ../i386
MISC = $(I386)/misc
RTS = $(I386)/rts
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
		   $(MISC)/io_inb.o \
		   $(MISC)/io_inw.o \
		   $(MISC)/io_inl.o \
		   $(MISC)/io_outb.o \
		   $(MISC)/io_outw.o \
		   $(MISC)/io_outl.o \
		   $(MISC)/io_intr.o \
		   $(RTS)/_ipc.o \
		   $(POSIX)/sigset.o
		

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

