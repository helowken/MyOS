	.code16gcc
	.text
movw	%cs, %ax
movw	%ax, %ds
movw	%ax, %es
cld

call	main


halt:
	jmp halt



