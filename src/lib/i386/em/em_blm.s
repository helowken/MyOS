	.text
	.globl	.blm
	.type	.blm, @function
.blm:
	movl	%esp, %ebx
	movl	%esi, %eax
	movl	%edi, %edx
	movl	4(%ebx), %edi
	movl	8(%ebx), %esi
	rep	movsb
	movl	%eax, %esi
	movl	%edx, %edi
	retl	$8
