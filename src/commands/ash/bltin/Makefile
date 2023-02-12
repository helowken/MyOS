MY_HOME = ../../..
include $(MY_HOME)/common.mk
LIBS = -L$(MY_HOME)/lib -lmyc -le
STACK_SIZE = 32kw
HEADERS += ../shell.h
CLEAN_FILES = operators.h operators.c

all: expr.bin \
	 echo.bin

mkexpr: mkexpr.sh
	./$<

operators.c operators.h: mkexpr.sh unary_op binary_op 
	./$<

sys_expr.o: operators.h

expr.bin: $(CRT_ENTRY) sys_expr.o operators.o stalloc.o error.o
	$(call mkbin,$^,$@,$(STACK_SIZE),debug_expr.bin)

echo.bin: $(CRT_ENTRY) sys_echo.o 
	$(call mkbin,$^,$@,$(STACK_SIZE),debug_echo.bin)

disasm: 
	$(call disasmCode,debug_$(NAME).bin,32)

clean: 
	$(call cleanCommon)
	rm -f $(CLEAN_FILES)
