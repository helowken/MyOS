print_string:
	pusha
	mov ah, 0x0E

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


println_string:
	call print_string
	call println
	ret


println:
	pusha

	mov ah, 0x0E
	mov al, 0xa
	int 0x10
	mov al, 0xd
	int 0x10

	popa
	ret



