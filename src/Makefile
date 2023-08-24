DIRS = lib \
	   tools \
	   kernel \
	   servers \
	   drivers \
	   boot \
	   commands \
	   test

DIR_ENTRY = entry

include funcs.mk

all:
	$(call mkDirs,$(DIRS))

clean: 
	$(call cleanDirs,$(DIRS))
	rm -f $(DIR_ENTRY)/*.o
