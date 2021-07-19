[bits 16]
section .text

global display

mov ax, cs
mov ds, ax
mov es, ax

;push message
;call display

[extern main]
call main
jmp $


display:
	push	ebp
	mov bx, [esp+0x8]
	call print_string
	pop ebp
	ret
	

message db 'abcdefg', 0

%include "print/print_string.asm"
%include "print/print_hex.asm"
