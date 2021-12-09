all:
	cd kernel && make
	cd servers && make
	cd boot && make install

clean:
	cd kernel && make clean
	cd servers && make clean
	cd boot && make clean

