MY_HOME = ../..
include $(MY_HOME)/common.mk
CFLAGS += -D_MINIX -D_POSIX_SOURCE

all: cron.bin \
	 crontab.bin

cron.bin: $(CRT_ENTRY) cron.o tab.o misc.o
	$(call mkbin,$^,$@,8kw,debug_$@)

crontab.bin: $(CRT_ENTRY) crontab.o tab.o misc.o
	$(call mkbin,$^,$@,4kw,debug_$@)

clean: 
	$(call cleanCommon)

disasm: 
	$(MK) NAME=$(NAME) disasm

