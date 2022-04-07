MAKE_M16 = $(MAKE) MODE=M16
DIR_M16 = m16

all:
	mkdir -p $(DIR_M16)
	cd ansi && $(MAKE) && $(MAKE_M16)
	cd syslib && $(MAKE) && $(MAKE_M16)
	cd sysutil && $(MAKE) && $(MAKE_M16)
	cd timers && $(MAKE) && $(MAKE_M16)

clean:
	mkdir -p $(DIR_M16)
	cd ansi && $(MAKE) clean
	cd syslib && $(MAKE) clean
	cd sysutil && $(MAKE) clean
	cd timers && $(MAKE) clean
