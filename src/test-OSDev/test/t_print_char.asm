[bits 16]
[org 0x7c00]

mov ah, 0xE

mov al, [the_secret]
int 0x10

jmp $

the_secret db 'X'

times 510 - ($-$$) db 0

dw 0xaa55
