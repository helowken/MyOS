MY_HOME = ../..
MK = $(MAKE) -f $(MY_HOME)/bin.mk MY_HOME=$(MY_HOME)
MAKEFLAGS += --no-print-directory
4kw_bins = loadkeys \
		   readclock
all_bins = $(4kw_bins)
include $(MY_HOME)/funcs.mk

all: 
	$(call mkBins,$(4kw_bins),4kw)

clean: 
	$(call cleanBins,$(all_bins))

disasm: 
	$(MK) NAME=$(NAME) disasm
