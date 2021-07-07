section .text

mov ax, LOAD_SEG
mov ds, ax

mov bx, MSG
call println_string

jmp $


%include "print/print_string.asm"

LOAD_SEG	equ	0x1000	; 0x1000:0x0 is where this code is loaded

MSG db 'Hello, world.', 0
