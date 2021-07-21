	.code16gcc
	.text
movw	%cs, %ax
movw	%ax, %ds
movw	%ax, %es		# Set es = ds = cs
cld						

calll	printLowMem
calll	printSepLine
calll	printE820Mem
calll	printSepLine

calll	boot


halt:
	jmp halt


# Functions:
	.globl	kputc
	.type	text, @function
kputc:
	movb	4(%esp), %al
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
	retl

	.globl	print
	.type	text, @function
print:
	movl	4(%esp), %ecx
.print_loop:
	movb	(%ecx), %al
	testb	%al, %al
	je	.end
	movb	$0xe, %ah
	int	$0x10
	inc	%ecx
	jmp	.print_loop
.end:
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
	movl	4(%esp), %eax
	movl	%ss:(%eax), %eax
	retl
	
