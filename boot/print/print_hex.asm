print_hex:
	pusha
	mov	cx, 4
	push bx
	add bx, HEX_OUT + 5  ; alternative 2

print_hex_loop:
	mov ax, dx
	and ax, 0xF
	add al, 0x30  ; for '0'
	cmp al, 0x39  ; for '9'
	jle print_hex_step2
	add al, 7  ; 'A'=65, '0'=48, 'A'-'0'=17, 0xA=10, so 17-10=7

print_hex_step2:
	mov [bx], al
	dec cx
	cmp cx, 0
	je print_hex_end

	shr dx, 4
	dec bx  ; alternative 2
	jmp print_hex_loop

print_hex_end:
	pop bx
	add bx, HEX_OUT
	call print_string

	popa
	ret


println_hex:
	call print_hex
	call println
	ret


HEX_OUT: 
	db '0x0000', 0

