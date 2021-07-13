section .text
;[extern main]
;jmp 0x0:main


global test

test:
	mov ah, 0x0E
	mov al, bl
	int 0x10
	ret
