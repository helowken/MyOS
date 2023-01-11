MY_HOME = ../..
include $(MY_HOME)/common.mk
CFLAGS += -D_MINIX -D_POSIX_SOURCE 
LIBS = -L$(MY_HOME)/lib \
	   -lmysys -lmysysutil -lmytimers -lmyc
DST_DIR = $(RES_DIR)/bin
START_FILE = $(MY_HOME)/entry/crtso.o
BIN = $(NAME).bin

all: $(BIN)

$(BIN): $(START_FILE) $(NAME).o
	$(call link,$^,$(LIBS),$@) 
	$(call setStack,$(STACK_SIZE),$@)
	$(call copyTo,$@,$(DST_DIR))

clean:
	rm -f $(NAME).o $(BIN) $(DST_DIR)/$(BIN)

