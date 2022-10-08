	.text
crtso:
	xorl	%ebp, %ebp			# Clear for backtrace of core files	
	movl	(%esp), %eax		# argc
	leal	4(%esp), %edx		# argv
	leal	8(%esp, %eax, 4), %ecx	# envp

# Test if environ is in the initialized data area and is set to our
# magic number. If so then it is not redefined by the user.
	movl	$environ, %ebx
	cmpl	$edata, %ebx		# Within initialized data?
	jae	0f
	testb	$3, %bl				# Aligned?
	jnz	0f
	cmpl	$0x53535353, (%ebx)	# Is it our environ?
	jne 0f
	movl	%ebx, _penviron		# _penviron = &environ;
0:
	movl	_penviron, %ebx
	movl	%ecx, (%ebx)		# *_penviron = envp;

	pushl	%ecx		# Push envp	
	pushl	%edx		# Push argv
	pushl	%eax		# Push argc

# TODO EN bit of MSW

	calll	main		# main(argc, argv, envp)

	pushl	%eax		# Push exit status
	calll	exit

	hlt					# Force a trap if exit fails


	.section	.data
_penviron:
	.value	_penvp		# Pointer to environ, or hidden pointer


	.section	.bss
	.lcomm	_penvp, 4	# Hidden environment vector
