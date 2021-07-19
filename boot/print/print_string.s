	.code16gcc
	.text
	.globl	print
	.type	text, @function
print:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%ebx
	movl	8(%ebp), %ebx
.print_loop:
	movb	(%ebx), %al
	testb	%al, %al
	je	.print_end
	movb	$0xe, %ah
	int	$0x10
	inc	%ebx
	jmp	.print_loop
.print_end:
	popl	%ebx
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

