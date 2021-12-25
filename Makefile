all:
	cd lib && $(MAKE)
	cd kernel && $(MAKE)
	cd servers && $(MAKE)
	cd boot && $(MAKE) install

clean:
	cd lib && $(MAKE) clean
	cd kernel && $(MAKE) clean
	cd servers && $(MAKE) clean
	cd boot && $(MAKE) clean

