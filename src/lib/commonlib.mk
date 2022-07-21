include $(MY_HOME)/common.mk
CFLAGS += -D_MINIX -D_POSIX_SOURCE
CFLAGS_16 = $(CFLAGS) -D_M16
LIB_DIR = $(MY_HOME)/lib
LIB_M32 = $(LIB_DIR)/lib$(LIB_NAME).a
LIB_M16 = $(LIB_DIR)/m16/lib$(LIB_NAME).a
DIR_16 = b16

ifdef NEED_16
$(shell mkdir -p $(DIR_16))
endif


$(DIR_16)/%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS_16) $< -o $@

$(LIB_M16): $(OBJS_M16)
	ar rcs $@ $(OBJS_M16)

$(LIB_M32): $(OBJS_M32)
	ar rcs $@ $(OBJS_M32)

clean16:
	rm -f $(DIR_16)/*.o

clean32: 
	rm -f *.o

