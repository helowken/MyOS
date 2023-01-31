# mkDirs(dirs)
define mkDirs 
	@$(foreach d, $(1), cd $(d) && $(MAKE) && cd ..;)
endef


# cleanDirs(dirs)
define cleanDirs
	@$(foreach d, $(1), cd $(d) && $(MAKE) clean && cd ..;)
endef


# mkBins(bins, stackSize)
define mkBins
	@$(foreach bin, $(1), $(MK) NAME=$(bin) STACK_SIZE=$(2);)
endef


# cleanBins(bins)
define cleanBins
	@$(foreach bin, $(1), $(MK) NAME=$(bin) clean;)
endef
