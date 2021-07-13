# $@ = target file
# $< = first dependency
# $^ = all dependencies

OS_IMAGE=os-image
ARGS="file=$(OS_IMAGE),format=raw,index=0,if=floppy"

all: clean run

boot_sect.bin: boot_sect.asm
	nasm -f bin $< -o $@

kernel_entry.o: kernel_entry.asm
	nasm -f elf -o $@ $<

kernel.o: kernel.c
	gcc -m32 -ffreestanding -c $< -o $@

kernel.bin: kernel_entry.o kernel.o
	ld -o $@ -Ttext 0x1000 $^ --oformat binary -melf_i386

$(OS_IMAGE): boot_sect.bin kernel.bin 
	cat $^ > $@

run: $(OS_IMAGE)
	qemu-system-i386 -drive $(ARGS)

clean:
	rm *.bin *.o
