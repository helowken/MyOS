all: compile

compile:
	cd tools && $(MAKE)
	cd lib && $(MAKE)
	cd kernel && $(MAKE)
	cd servers && $(MAKE)
	cd drivers && $(MAKE)
	cd boot && $(MAKE) 

clean:
	cd tools && $(MAKE) clean
	cd lib && $(MAKE) clean
	cd kernel && $(MAKE) clean
	cd servers && $(MAKE) clean
	cd drivers && $(MAKE) clean
	cd boot && $(MAKE) clean

