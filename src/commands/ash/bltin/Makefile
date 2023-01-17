MY_HOME = ../../..
include $(MY_HOME)/common.mk
LIBS = -L$(MY_HOME)/lib -lmyc -le
DST_DIR = $(RES_DIR)/bin
STACK_SIZE = 32kw
HEADERS += ../shell.h
CLEAN_FILES = operators.h operators.c
BINS = expr.bin

all: $(BINS)

mkexpr: mkexpr.sh
	./$<

operators.c operators.h: mkexpr.sh unary_op binary_op 
	./$<

sys_expr.o: operators.h

expr.bin: $(CRT_ENTRY) sys_expr.o operators.o stalloc.o error.o
	$(call mkbin,$^,$@,$(STACK_SIZE),$(DST_DIR))

clean: 
	$(call cleanCommon)
	rm -f $(CLEAN_FILES)
	cd $(DST_DIR) && rm -f $(BINS)
