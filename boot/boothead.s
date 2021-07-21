	.code16gcc
	.text
movw	%cs, %ax
movw	%ax, %ds
movw	%ax, %es		# Set es = ds = cs
cld						

#calll	printLowMem
#calll	detectE820Mem

calll	boot

halt:
	jmp halt

