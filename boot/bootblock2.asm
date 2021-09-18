[bits 16]

section .text


	mov bx, LOAD_OFF+ERR_READ
	call print_string
	jmp $

%include "print/print_string.asm"

LOAD_OFF	equ 0x7C00	; 0x0000:LOAD_OFF is where this code is loaded
ERR_READ	db	'hihihi: ', 0
