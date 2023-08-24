MY_HOME = ..
MK = $(MAKE) -f $(MY_HOME)/bin.mk MY_HOME=$(MY_HOME)
MAKEFLAGS += --no-print-directory
10kw_bins = test1 \
			test2 \
			test3 \
			test4 \
			test5 \
			test6 \
			test7 \
			test8 \
			test9 \
			test11 \
			t11a \
			t11b \
			test12 \
			test13 \
			test14 \
			test15 \
			test16 \
			test17 \
			test19 \
			test21 \
			test22 \
			test23 \
			test25

32kw_bins = test20 \
			test24

all_bins = $(10kw_bins) \
		   $(32kw_bins)

include $(MY_HOME)/funcs.mk

all: 
	$(call mkBins,$(10kw_bins),10kw)
	$(call mkBins,$(32kw_bins),32kw)

clean: 
	$(call cleanBins,$(all_bins))

disasm: 
	$(MK) NAME=$(NAME) disasm
