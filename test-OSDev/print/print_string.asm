print_string:
	pusha
	mov ah, 0xE

print_string_loop:
	mov al, [bx]
	cmp al, 0
	je print_end
	int 0x10

	inc bx
	jmp print_string_loop

print_end:	
	popa
	ret


print_nl:
	pusha

	mov ah, 0xE
	mov al, 0xa
	int 0x10
	mov al, 0xd
	int 0x10

	popa
	ret



