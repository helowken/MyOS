	.text
	.globl	.blm
	.type	.blm, @function
# ecx: count in double words 
.blm:
	movl	%esp, %ebx
	movl	%esi, %eax		# save esi
	movl	%edi, %edx		# save edi
	movl	4(%ebx), %edi
	movl	8(%ebx), %esi
	rep	movsd
	movl	%eax, %esi		# restore esi
	movl	%edx, %edi		# restore edi
	retl	$8
