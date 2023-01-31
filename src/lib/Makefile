DIR_16 = m16
DIRS = ansi \
	   other \
	   posix \
	   i386 \
	   syslib \
	   sysutil \
	   timers \
	   rts \
	   stdio \
	   fphook
include ../funcs.mk

all:
	$(call mkDirs,$(DIRS))

clean: 
	$(call cleanDirs,$(DIRS))
	rm -f *.a
	rm -f $(DIR_16)/*.a
