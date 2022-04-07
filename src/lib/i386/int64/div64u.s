# unsigned long div64u(u64_t i, unsigned j);
# unsigned rem64u(u64_t i, unsigned j);
#
# Stack Layout
#   esp + 12:  j
#   esp +  8:  high 32-bit of i
#   esp +  4:  low  32-bit of i
#	esp +  0:  retAddr
#
# edx:eax / memory => eax(quotient), edx(remainder)
# Since the return type is unsigned long, the quotient in high 32-bit is thrown away.
#
# i = (i-high << 32) + i-low
# i-high = quotient * j + remainder
# i / j = (quotient << 32) + ((remainder << 32) + i-low) / j
#
# Since we don't need quotient in high 32-bit, then only compute: 
#  ((remainder << 32) + i-low) / j

	.text
	.globl	div64u
	.type	div64u, @function
div64u:
	xorl	%edx, %edx			# clear edx
	movl	8(%esp), %eax		# eax = high 32-bit of i
	divl	12(%esp)			# eax = quotient of high 32-bit,
								# edx = remainder of high 32-bit
	movl	4(%esp), %eax		# eax = low 32-bit of i
								# Now edx:eax = (remainder << 32) + i-low
	divl	12(%esp)			# eax = quotient of low 32-bit, 
								# edx = remainder of low 32-bit
	retl						

	.globl	rem64u
	.type	rem64u, @function
rem64u:
	popl	%ecx				# ecx = retAddr
	calll	div64u
	movl	%edx, %eax			# eax = edx = remainder of low 32-bit
	jmpl	*%ecx				# Return to the retAddr


