# $@ = target file
# $< = first dependency
# $^ = all dependencies

all: clean run

SOURCE=$(FILE).asm
BIN=$(FILE).bin
ARGS="file=$(BIN),format=raw,index=0,if=floppy"

$(BIN): $(SOURCE)
	nasm -f bin $< -o $@

run: $(BIN)
	qemu-system-i386 -drive $(ARGS)

clean:
	rm -f $(BIN)
