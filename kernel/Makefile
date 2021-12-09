MY_HOME = ..
include $(MY_HOME)/common.mk
SYSTEM = mysystem
LIB_SYSTEM = lib$(SYSTEM).a
LIBS = -L$(MY_HOME)/lib -L. \
	   -l$(SYSTEM) -lmysysutil -lmytimers -lmyc 

OBJS = mpx.o start.o protect.o klib386.o table.o main.o proc.o \
	   i8259.o exception.o system.o clock.o utility.o
KERNEL = kernel.bin

all: $(KERNEL)

$(LIB_SYSTEM):
	cd system && make

$(KERNEL): $(OBJS) $(LIB_SYSTEM)
	$(call link,$(OBJS),$(LIBS),$@) 
	$(call createDebug,$@,$(DEBUG))

test: entry.o test.o
	$(LD) $^ -Ttext 0x0 -m elf_i386 -nostdlib -o $(KERNEL)

clean: 
	$(call cleanCommon)
	cd system && make clean

disasm: $(DEBUG)
	$(call disasmCode,$<,32)


