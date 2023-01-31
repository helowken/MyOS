MK = $(MAKE) -f ../bin.mk
MAKEFLAGS += --no-print-directory
4kw_bins = loadkeys \
		   readclock
all_bins = $(4kw_bins)
include ../../funcs.mk

all: 
	$(call mkBins,$(4kw_bins),4kw)

clean: 
	$(call cleanBins,$(all_bins))

disasm: 
	$(MK) NAME=$(NAME) disasm
