MY_HOME = ../..
MK = $(MAKE) -f $(MY_HOME)/bin.mk MY_HOME=$(MY_HOME)
MAKEFLAGS += --no-print-directory

1kw_bins = update

4kw_bins = sysenv \
	       env \
		   printenv \
		   sleep \
		   intr \
		   date \
		   printroot \
		   cat \
		   mount \
		   umount \
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
		   getty \
		   login \
		   pwdauth \
		   which \
		   basename \
		   dirname \
		   rmdir \
		   cmp \
		   wc \
		   whoami \
		   who \
		   id \
		   passwd \
		   su \
		   df \
		   sync \
		   chroot \
		   mknod \
		   tr \
		   testAAA

5kw_bins = last

8kw_bins = sed

16kw_bins = cp \
			chown \
			du

20kw_bins = ls \
			dd 

25kw_bins = find

32kw_bins = grep

4096k_bins = fsck

all_bins = $(1kw_bins) \
		   $(4kw_bins) \
		   $(5kw_bins) \
		   $(8kw_bins) \
		   $(16kw_bins) \
		   $(20kw_bins) \
		   $(25kw_bins) \
		   $(32kw_bins) \
		   $(4096k_bins)
include $(MY_HOME)/funcs.mk

all: 
	$(call mkBins,$(1kw_bins),1kw)
	$(call mkBins,$(4kw_bins),4kw)
	$(call mkBins,$(5kw_bins),5kw)
	$(call mkBins,$(8kw_bins),8kw)
	$(call mkBins,$(16kw_bins),16kw)
	$(call mkBins,$(20kw_bins),20kw)
	$(call mkBins,$(25kw_bins),25kw)
	$(call mkBins,$(32kw_bins),32kw)
	$(call mkBins,$(4096k_bins),4096k)

clean: 
	$(call cleanBins,$(all_bins))

disasm: 
	$(MK) NAME=$(NAME) disasm

