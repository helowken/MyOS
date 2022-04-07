[bits 16]

mov ah, 0xE

mov al, [the_secret]
int 0x10

mov bx, 0x7C0  ; must use a general purpose register to transfer the value to segment register
mov ds, bx
mov al, [the_secret]
int 0x10

mov al, [es:the_secret]
int 0x10

mov bx, 0x7c0
mov es, bx
mov al, [es:the_secret]
int 0x10

jmp $

the_secret:
	db 'X'

times 510 - ($-$$) db 0
dw 0xaa55
