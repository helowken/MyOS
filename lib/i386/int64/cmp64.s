# int cmp64(u64_t i, u64_t j);
# int cmp64u(u64_t i, unsigned j);
# int cmp64ul(u64_t i, unsigned long j);
#
# Stack Layout
# 
#   esp + 16:  high 32-bit of  j
#   esp + 12:  low  32-bit of  j
#   esp +  8:  high 32-bit of i
#   esp +  4:  low  32-bit of i
#	esp +  0:  retAddr

	.text
	.globl	cmp64
	.type	cmp64, @function
cmp64:	
	movl	%esp, %ecx
_cmp:
	xorl	%eax, %eax			# eax = 0
	movl	4(%ecx), %edx		# edx = low 32-bit of i
	subl	12(%ecx), %edx		# edx -= low 32-bit of  j (if edx < 0, CF = 1)
	movl	8(%ecx), %edx		# edx = high 32-bit of i
	sbbl	16(%ecx), %edx		# edx -= (CF + high 32-bit of j)  (if edx < 0, CF = 1)
	sbbl	%eax, %eax			# eax -= CF => if i < j then eax -= 1

	movl	12(%ecx), %edx		# edx = low 32-bit of j
	subl	4(%ecx), %edx		# edx -= low 32-bit of  i (if edx < 0, CF = 1)
	movl	16(%ecx), %edx		# edx = high 32-bit of j
	sbbl	8(%ecx), %edx		# edx -= (CF + high 32-bit of i)  (if edx < 0, CF = 1)
	adcl	$0, %eax			# eax += CF => if j < i then eax += 1
	retl						

	.globl	cmp64u
	.type	cmp64u, @function
cmp64u:

	.globl	cmp64ul
	.type	cmp64ul, @function
cmp64ul:
	movl	%esp, %ecx
	pushl	16(%ecx)
	movl	$0, 16(%ecx)
	calll	_cmp
	popl	16(%ecx)
	retl


