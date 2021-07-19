	.code16gcc
	.text
movw	%cs, %ax
movw	%ax, %ds
movw	%ax, %es
cld


pushl	g
calll	printIntHex
addl	$4, %esp


calll	test	


halt:
	jmp halt

	.section	.rodata
	.align	4
	.type	g, @object
	.size	g, 4
g:
	.long	305441741
