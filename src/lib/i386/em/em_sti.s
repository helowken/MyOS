	.text
	.globl	.sti
	.type	.sti, @function
# ecx: count in bytes
# ebx: address
# save edi/esi, they might be register variables.

.sti:
# Only called with count >> 4
	movl	%edi, %edx		# save edi
	movl	%ebx, %edi		# edi = address
	popl	%ebx			# ebx = retAddr
	sarl	$2, %ecx		# ecx /= 4
1:
	movl	%esi, %eax		# save esi
	movl	%esp, %esi		# esi = esp
	rep	movsd				# do copy
	movl	%esi, %esp		# esp -= 4 * #bytes
	movl	%edx, %edi		# restore edi
	movl	%eax, %esi		# restore esi
	jmp	*%ebx				# jump back to the caller
