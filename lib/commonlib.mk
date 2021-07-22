MY_HOME = ../..
include $(MY_HOME)/common.mk
LIB_M32 = ../lib$(LIB_NAME).a
LIB_M16 = ../m16/lib$(LIB_NAME).a
LIB = $(LIB_M32)
OBJS = $(OBJS_M32)
ifeq ($(MODE), M16)
	LIB = $(LIB_M16)
	OBJS = $(OBJS_M16)
endif

all: $(LIB)

clean: cleanObjs
	rm -f $(LIB_M32) $(LIB_M16)

cleanObjs:
	rm -f *.o

$(LIB): cleanObjs $(OBJS)
	ar rcs $@ *.o

