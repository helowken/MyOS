mk_all:
	@$(foreach lib, $(MODULES), cd $(lib) && $(MAKE) && cd ..;)

mk_clean:
	@$(foreach lib, $(MODULES), cd $(lib) && $(MAKE) clean && cd ..;)



