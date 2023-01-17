NAME = cmos
OBJS = $(NAME).o
STACK_SIZE = 1024w
include ../driver.mk
DST_DIR = $(RES_DIR)/sbin
DST_BIN = $(DST_DIR)/$(DRIVER)

all: $(DST_BIN)

$(DST_BIN): $(DRIVER)
	$(call copyTo,$(DRIVER),$(DST_DIR))

clean:
	$(call cleanCommon)
	rm -f $(DST_BIN)
