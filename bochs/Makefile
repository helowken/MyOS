BOCHS = bochs-20211104
TAR = $(BOCHS).tar.gz
CONF = my.conf.linux
DIR_BUILD = $(shell pwd)/build
DIR_BOCHS = $(DIR_BUILD)/$(BOCHS)
DIR_BUILD_NORMAL = $(DIR_BUILD)/normal
DIR_BUILD_GDB = $(DIR_BUILD)/gdb
D_UNZIP = $(DIR_BUILD)/done_unzip
D_NORMAL = $(DIR_BUILD)/done_normal
D_GDB = $(DIR_BUILD)/done_gdb


all: build-normal build-gdb


$(D_UNZIP):
	mkdir -p $(DIR_BUILD)
	tar xvf $(TAR) -C $(DIR_BUILD)
	cp $(CONF) $(DIR_BOCHS)
	touch $@

unzip: $(D_UNZIP)
	:

$(D_NORMAL): 
ifneq (,$(wildcard $(DIR_BOCHS)/Makefile))
	cd $(DIR_BOCHS) && make clean 
endif
	$(call compile,normal,$(DIR_BUILD_NORMAL))
	touch $@

build-normal: unzip $(D_NORMAL)
	:


$(D_GDB): 
ifneq (,$(wildcard $(DIR_BOCHS)/Makefile))
	cd $(DIR_BOCHS) && make clean 
endif
	$(call compile,gdb,$(DIR_BUILD_GDB))
	touch $@

build-gdb: unzip $(D_GDB)
	:


clean: 
	rm -rf $(DIR_BUILD)


.PHONY: clean 


define compile
	cd $(DIR_BOCHS) && \
	./$(CONF) $(1) $(2) && \
	make && \
	make install
endef


