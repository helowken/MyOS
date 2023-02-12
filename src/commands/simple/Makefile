MK = $(MAKE) -f ../bin.mk
MAKEFLAGS += --no-print-directory
4kw_bins = sysenv \
		   sleep \
		   intr \
		   date \
		   printroot \
		   cat \
		   mount \
		   umount \
		   cp \
		   stat \
		   pwd \
		   mkdir \
		   chmod \
		   testAAA 

20kw_bins = ls

all_bins = $(4kw_bins) $(20kw_bins)
include ../../funcs.mk

all: 
	$(call mkBins,$(4kw_bins),4kw)
	$(call mkBins,$(20kw_bins),20kw)

clean: 
	$(call cleanBins,$(all_bins))

disasm: 
	$(MK) NAME=$(NAME) disasm

