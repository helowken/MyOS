MAKE_M16 = $(MAKE) MODE=M16

all:
	cd ansi && $(MAKE) && $(MAKE_M16)
	cd sysutil && $(MAKE) && $(MAKE_M16)
	cd timers && $(MAKE) && $(MAKE_M16)

clean:
	cd ansi && $(MAKE) clean
	cd syslib && $(MAKE) clean
	cd sysutil && $(MAKE) clean
	cd timers && $(MAKE) clean
