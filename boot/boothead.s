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

	.globl	kputc
	.type	text, @function
kputc:
	pushl	%ebp
	movl	%esp, %ebp
	movb	8(%ebp), %al
	testb	%al, %al
	jz	.noChar
	movb	$0xe, %ah
	cmpb	$0xA, %al
	jnz	.putChar
	movb	$0xD, %al
	int	$0x10
	movb	$0xA, %al
.putChar:
	int	$0x10
.noChar:
	leave
	retl
	.globl	print
	.type	text, @function
print:
	pushl	%ebp
	movl	%esp, %ebp
	movl	8(%ebp), %ecx
.print_loop:
	movb	(%ecx), %al
	testb	%al, %al
	je	.end
	movb	$0xe, %ah
	int	$0x10
	inc	%ecx
	jmp	.print_loop
.end:
	leave
	retl
	.globl	println
	.type	text, @function
println:
	pushl	%ebp
	movl	%esp, %ebp
	movl	0x8(%ebp), %eax
	pushl	%eax
	calll	print
	movb	$0xe, %ah
	movb	$0xa, %al
	int	$0x10
	movb	$0xd, %al
	int	$0x10
	leave
	retl
	.globl	derefSp
	.type	text, @function
derefSp:
	pushl	%ebp
	movl	%esp, %ebp
	movl	8(%ebp), %eax
	movl	%ss:(%eax), %eax
	leave
	retl
	
