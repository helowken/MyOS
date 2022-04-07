MY_HOME = ../..
include $(MY_HOME)/common.mk
LIB_M32 = ../lib$(LIB_NAME).a
LIB_M16 = ../m16/lib$(LIB_NAME).a
LIB = $(LIB_M32)
ifeq ($(MODE), M16)
	LIB = $(LIB_M16)
endif

all: $(LIB)

clean: clean_m16 clean_m32

clean_m16: clean_m16_objs
	rm -f $(LIB_M16)

clean_m16_objs:
	rm -f $(OBJS_M16)

clean_m32: clean_m32_objs
	rm -f $(LIB_M32)

clean_m32_objs:
	rm -f $(OBJS_M32)

$(LIB_M16): clean_m16_objs $(OBJS_M16)
	ar rcs $@ $(OBJS_M16)

$(LIB_M32): clean_m32_objs $(OBJS_M32)
	ar rcs $@ $(OBJS_M32)

