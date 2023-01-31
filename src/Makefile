DIRS = tools \
	   lib \
	   kernel \
	   servers \
	   drivers \
	   boot \
	   commands

DIR_ENTRY = entry

include funcs.mk

all:
	$(call mkDirs,$(DIRS))

clean: 
	$(call cleanDirs,$(DIRS))
	rm -f $(DIR_ENTRY)/*.o
