	.text
	.globl	.sti
	.type	.sti, @function
.sti:
# Only called with count >> 4
	movl	%edi, %edx
	movl	%ebx, %edi
	popl	%ebx
	sarl	$2, %ecx
1:
	movl	%esi, %eax
	movl	%esp, %esi
	rep	movsb
	movl	%esi, %esp
	movl	%edx, %edi
	movl %eax, %esi
	jmp	*%ebx
