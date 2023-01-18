MY_HOME = ../..
include $(MY_HOME)/common.mk
CFLAGS += -DSHELL -D_MINIX -D_POSIX_SOURCE 
LIBS = -L$(MY_HOME)/lib -lmyc -le
DST_DIR = $(RES_DIR)/bin
PB_DIR = prebuild
BLTIN_DIR = bltin

SRCS = builtins.c cd.c error.c eval.c exec.c expand.c input.c \
	   jobs.c mail.c main.c memalloc.c miscbltin.c mystring.c nodes.c \
	   options.c parser.c redir.c signames.c syntax.c trap.c \
	   output.c var.c

OBJS = builtins.o cd.o error.o eval.o exec.o expand.o input.o \
	   jobs.o mail.o main.o memalloc.o miscbltin.o mystring.o nodes.o \
	   options.o parser.o redir.o signames.o syntax.o trap.o \
	   output.o var.o init.o \
	   $(BLTIN_DIR)/echo.o $(BLTIN_DIR)/expr.o $(BLTIN_DIR)/operators.o

CLEAN_FILES = builtins.c builtins.h init.c signames.c signames.h \
			  syntax.c syntax.h token.h nodes.c nodes.h \
			  $(BLTIN_DIR)/operators.h $(BLTIN_DIR)/operators.c

STACK_SIZE = 100k
BIN = sh.bin

all: compile $(BIN) mkbltin

$(OBJS): init.h syntax.h signames.h nodes.h $(BLTIN_DIR)/*.h

init:
	cd $(PB_DIR) && $(MAKE)

mkbltin:
	cd $(BLTIN_DIR) && $(MAKE)

compile: init token.h $(BLTIN_DIR)/operators.h

$(BIN): $(CRT_ENTRY) $(OBJS)
	$(call mkbin,$^,$@,$(STACK_SIZE),$(DEBUG),$(DST_DIR))

init.c: $(PB_DIR)/mkinit.bin $(SRCS)
	cd $(PB_DIR) && $(MAKE) mkinit ARGS="$(SRCS)"

syntax.c syntax.h: prebuild/mksyntax.bin
	cd $(PB_DIR) && $(MAKE) mksyntax

signames.c signames.h: $(PB_DIR)/mksignames.bin
	cd $(PB_DIR) && $(MAKE) mksignames

token.h: $(PB_DIR)/mktokens.sh
	cd $(PB_DIR) && $(MAKE) mktokens

parser.o: token.h

nodes.c nodes.h: $(PB_DIR)/mknodes.bin $(PB_DIR)/nodetypes $(PB_DIR)/nodespat.c
	cd $(PB_DIR) && $(MAKE) mknodes

builtins.c builtins.h: $(PB_DIR)/mkbuiltins.sh $(PB_DIR)/builtins.table
	cd $(PB_DIR) && $(MAKE) mkbuiltins

$(BLTIN_DIR)/operators.c $(BLTIN_DIR)/operators.h: $(BLTIN_DIR)/mkexpr.sh $(BLTIN_DIR)/unary_op $(BLTIN_DIR)/binary_op
	cd $(BLTIN_DIR) && $(MAKE) mkexpr

$(BLTIN_DIR)/expr.o: $(BLTIN_DIR)/operators.h

clean: 
	cd $(PB_DIR) && $(MAKE) clean
	cd $(BLTIN_DIR) && $(MAKE) clean
	rm -f $(CLEAN_FILES)
	rm -f $(DST_DIR)/$(BIN)
	$(call cleanCommon)

disasm: $(DEBUG)
	$(call disasmCode,$<,32)


