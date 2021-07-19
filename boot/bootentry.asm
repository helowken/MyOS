[bits 16]

global display

extern main

section .text
mov ax, cs
mov ds, ax
mov es, ax

call main
call main

jmp $


display:
	push	ebp
	mov bx, [esp+0x8]
	call println_string
	pop ebp
	ret


%include "print/print_string.asm"
%include "print/print_hex.asm"

