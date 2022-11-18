MY_HOME = ../..
include $(MY_HOME)/common.mk
CFLAGS += -DSHELL -D_MINIX -D_POSIX_SOURCE 
LIBS = -L$(MY_HOME)/lib \
	   -lmysysutil -lmysys -lmytimers -lmyc -le
DST_DIR = $(MY_HOME)/tools/resources/bin

START_FILE = $(MY_HOME)/entry/crtso.o

SRCS = error.c eval.c exec.c expand.c input.c \
	   jobs.c mail.c main.c memalloc.c mystring.c nodes.c \
	   options.c parser.c redir.c signames.c syntax.c trap.c \
	   output.c var.c

OBJS = error.o eval.o exec.o expand.o input.o \
	   jobs.o mail.o main.o memalloc.o mystring.o nodes.o \
	   options.o parser.o redir.o signames.o syntax.o trap.o \
	   output.o var.o init.o

CLEAN_FILES = init.c signames.c signames.h syntax.c syntax.h token.h \
			  nodes.c nodes.h

STACK_SIZE = 102400
BIN = sh.bin
PREBUILD = prebuild

all: doInit $(BIN)

$(OBJS): init.h syntax.h signames.h nodes.h

doInit:
	cd $(PREBUILD) && $(MAKE)

$(BIN): $(START_FILE) $(OBJS)
	$(call link,$^,$(LIBS),$@) 
	$(call createDebug,$@,$(DEBUG))
	$(call setStack,$(STACK_SIZE),$(BIN))
	cp $@ $(DST_DIR)/$(NAME)

init.c: $(PREBUILD)/mkinit.bin $(SRCS)
	cd $(PREBUILD) && $(MAKE) mkinit ARGS="$(SRCS)"

syntax.c syntax.h: prebuild/mksyntax.bin
	cd $(PREBUILD) && $(MAKE) mksyntax

signames.c signames.h: $(PREBUILD)/mksignames.bin
	cd $(PREBUILD) && $(MAKE) mksignames

token.h: $(PREBUILD)/mktokens.sh
	cd $(PREBUILD) && $(MAKE) mktokens

parser.o: token.h

nodes.c nodes.h: $(PREBUILD)/mknodes.bin $(PREBUILD)/nodetypes $(PREBUILD)/nodespat.c
	cd $(PREBUILD) && $(MAKE) mknodes


clean: 
	cd $(PREBUILD) && $(MAKE) clean
	rm -f $(CLEAN_FILES)
	$(call cleanCommon)

disasm: $(DEBUG)
	$(call disasmCode,$<,32)
