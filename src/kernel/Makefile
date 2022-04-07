MY_HOME = ..
include $(MY_HOME)/common.mk
DIR_SYSTEM = system
SYSTEM = mysystem
LIB_SYSTEM = lib$(SYSTEM).a
LIBS = -L$(MY_HOME)/lib -L. \
	   -l$(SYSTEM) -lmysysutil -lmytimers -lmyc 

OBJS = mpx.o start.o protect.o klib386.o table.o main.o proc.o \
	   i8259.o exception.o system.o clock.o utility.o
KERNEL = kernel.bin

all: $(KERNEL)

$(LIB_SYSTEM): $(DIR_SYSTEM)/*.o
	cd $(DIR_SYSTEM) && make

$(KERNEL): $(OBJS) $(LIB_SYSTEM)
	$(call link,$(OBJS),$(LIBS),$@) 
	$(call createDebug,$@,$(DEBUG))

clean: 
	cd $(DIR_SYSTEM) && $(MAKE) clean
	$(call cleanCommon)

disasm: $(DEBUG)
	$(call disasmCode,$<,32)


