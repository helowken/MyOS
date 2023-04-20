MK = $(MAKE) -f ../bin.mk
MAKEFLAGS += --no-print-directory

1kw_bins = update

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
		   uname \
		   touch \
		   od \
		   usyslogd \
		   at \
		   kill \
		   mail \
		   testAAA

20kw_bins = ls \
			dd

4096k_bins = fsck

all_bins = $(1kw_bins) \
		   $(4kw_bins) \
		   $(20kw_bins) \
		   $(4096k_bins)
include ../../funcs.mk

all: 
	$(call mkBins,$(1kw_bins),1kw)
	$(call mkBins,$(4kw_bins),4kw)
	$(call mkBins,$(20kw_bins),20kw)
	$(call mkBins,$(4096k_bins),4096k)

clean: 
	$(call cleanBins,$(all_bins))

disasm: 
	$(MK) NAME=$(NAME) disasm

