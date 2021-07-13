[bits 16]
[org 0x7c00]

mov dx, 0x1fb6
call print_hex
call print_nl

mov dx, 0x9C70
call print_hex
call print_nl

jmp $

%include "../print/print_string.asm"
%include "../print/print_hex.asm"


times 510 - ($-$$) db 0
dw 0xaa55



