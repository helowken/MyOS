# $@ = target file
# $< = first dependency
# $^ = all dependencies

img=device/testOS.img
lib=../lib
all: clean install run

#for floppy:  ARGS="file=$(BIN),format=raw,index=0,if=floppy"
ARGS="file=$(img),format=raw,index=0,media=disk"

masterboot.bin: masterboot.asm
	nasm -f bin $< -o $@

installboot: installboot.c 
	gcc -o $@ $< -I$(lib) $(lib)/*.o

install: installboot masterboot.bin
	./installboot $(img) masterboot.bin

run: masterboot.bin
	qemu-system-i386 -drive $(ARGS)

clean:
	rm -f *.o *.bin instasllboot
