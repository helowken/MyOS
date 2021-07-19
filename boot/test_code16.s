	.file	"test_code16.c"
#APP
	.code16gcc

#NO_APP
	.text
	.type	calc, @function
calc:
.LFB0:
	.cfi_startproc
	pushl	%ebp
	.cfi_def_cfa_offset 8
	.cfi_offset 5, -8
	movl	%esp, %ebp
	.cfi_def_cfa_register 5
	movl	12(%ebp), %eax
	movl	8(%ebp), %edx
	movl	%eax, %ecx
	sarl	%cl, %edx
	movl	%edx, %eax
	movzbl	%al, %eax
	leal	48(%eax), %edx
	movl	12(%ebp), %eax
	movl	%eax, %ecx
	sall	%cl, %edx
	movl	%edx, %eax
	popl	%ebp
	.cfi_restore 5
	.cfi_def_cfa 4, 4
	ret
	.cfi_endproc
.LFE0:
	.size	calc, .-calc
	.type	convert, @function
convert:
.LFB1:
	.cfi_startproc
	pushl	%ebp
	.cfi_def_cfa_offset 8
	.cfi_offset 5, -8
	movl	%esp, %ebp
	.cfi_def_cfa_register 5
	subl	$16, %esp
	pushl	$0
	pushl	8(%ebp)
	call	calc
	addl	$8, %esp
	andl	$255, %eax
	movl	%eax, -4(%ebp)
	pushl	$8
	pushl	8(%ebp)
	call	calc
	addl	$8, %esp
	andl	$65280, %eax
	orl	%eax, -4(%ebp)
	pushl	$16
	pushl	8(%ebp)
	call	calc
	addl	$8, %esp
	andl	$16711680, %eax
	orl	%eax, -4(%ebp)
	pushl	$24
	pushl	8(%ebp)
	call	calc
	addl	$8, %esp
	andl	$-16777216, %eax
	movl	%eax, %edx
	movl	-4(%ebp), %eax
	orl	%edx, %eax
	movl	%eax, -4(%ebp)
	movl	-4(%ebp), %eax
	leave
	.cfi_restore 5
	.cfi_def_cfa 4, 4
	ret
	.cfi_endproc
.LFE1:
	.size	convert, .-convert
	.globl	main
	.type	main, @function
main:
.LFB2:
	.cfi_startproc
	leal	4(%esp), %ecx
	.cfi_def_cfa 1, 0
	andl	$-16, %esp
	pushl	-4(%ecx)
	pushl	%ebp
	.cfi_escape 0x10,0x5,0x2,0x75,0
	movl	%esp, %ebp
	pushl	%ecx
	.cfi_escape 0xf,0x3,0x75,0x7c,0x6
	subl	$4, %esp
	subl	$12, %esp
	pushl	$808464435
	call	print
	addl	$16, %esp
	nop
	movl	-4(%ebp), %ecx
	.cfi_def_cfa 1, 0
	leave
	.cfi_restore 5
	leal	-4(%ecx), %esp
	.cfi_def_cfa 4, 4
	ret
	.cfi_endproc
.LFE2:
	.size	main, .-main
	.section	.rodata
.LC0:
	.string	"111111"
	.text
	.globl	main2
	.type	main2, @function
main2:
.LFB3:
	.cfi_startproc
	pushl	%ebp
	.cfi_def_cfa_offset 8
	.cfi_offset 5, -8
	movl	%esp, %ebp
	.cfi_def_cfa_register 5
	subl	$8, %esp
	subl	$4, %esp
	pushl	8(%ebp)
	pushl	$6
	pushl	$.LC0
	call	display
	addl	$16, %esp
	subl	$12, %esp
	pushl	8(%ebp)
	call	convert
	addl	$16, %esp
	subl	$12, %esp
	pushl	%eax
	call	print
	addl	$16, %esp
	nop
	leave
	.cfi_restore 5
	.cfi_def_cfa 4, 4
	ret
	.cfi_endproc
.LFE3:
	.size	main2, .-main2
	.ident	"GCC: (Ubuntu 5.4.0-6ubuntu1~16.04.12) 5.4.0 20160609"
	.section	.note.GNU-stack,"",@progbits
