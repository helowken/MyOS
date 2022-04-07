[bits 16]

mov ah, 0xE

mov bx, 30

cmp bx, 4
jle le4
cmp bx, 40
jl	l40
mov al, 'C'
jmp print

le4:
	mov al, 'A'
	jmp print

l40:
	mov al, 'B'
	jmp print

print:
	int 0x10

jmp $

times 510 - ($-$$) db 0

dw 0xaa55
