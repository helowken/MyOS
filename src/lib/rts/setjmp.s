	.text

# __setjmp(jmp_buf env, int saveMask);
	.globl	__setjmp
	.type	__setjmp, @function
__setjmp:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%ebx				# save ebx

# Save regs
	pushfl						# off: 60, eflags
	pushl	%cs					# off: 56
	pushl	$0					# off: 52, pc, not use here

	pushl	4(%ebp)				# off: 48, retAddr
	pushl	%eax				# off: 44
	pushl	%ecx				# off: 40
	pushl	%edx				# off: 36
	pushl	%ebx				# off: 32
	leal	8(%ebp), %eax		# eax = parent esp = off(ebp + retAddr) = 8
	pushl	%eax				# off: 28, parent esp
	pushl	0(%ebp)				# off: 24, original ebp
	pushl	%esi				# off: 20
	pushl	%edi				# off: 16

	pushl	%ds					# off: 12
	pushl	%es					# off: 8
	pushl	%fs					# off: 4
	pushl	%gs					# off: 0

# Start to save information to env.
	movl	8(%ebp), %edx		# edx = env
	movl	12(%ebp), %ecx		# ecx = saveMask
	movl	%ecx, 0(%edx)		# env->__flags = saveMask
	cmpl	$0x0, %ecx			# if saveMask == 0	(not save sigmask)
	je	.I1_1					

# Save sigmask
	addl	$4, %edx			# edx = &(env->__mask)
	pushl	%edx			
	calll	__newSigset			# *(env->__mask) = original sigmask
	popl	%ecx				

.I1_1:
	movl	8(%ebp), %ebx		# ebx = env
	addl	$8, %ebx			# ebx = env->__regs
	movl	$64, %ecx			# ecx = 4 * 16 = 64 (16 regs)
	calll	.sti				# copy regs to env->__regs
	popl	%ebx				# restore ebx
	xorl	%eax, %eax			# must return 0
	leave
	retl


# void longjmp(jmp_buf env, int val);
	.globl	longjmp
	.type	longjmp, @function
longjmp:
	movl	4(%esp), %edx		# edx = env
	cmpl	$0, 0(%edx)			# if env->__flags == 0 (not restore sigmask)
	je	.I3_2

# Restore sigmask
	addl	$4, %edx			# edx = &(env->__mask)
	pushl	%edx
	calll	__oldSigset
	popl	%ecx

# Calculate return value
	movl	8(%esp), %edx		# edx = val
	testl	%edx, %edx
	jne	.I3_3
	incl	%edx
.I3_3:
	movl	%edx, %eax

# Restore regs
.I3_2:
	movl	4(%esp), %edx		# edx = env
	addl	$8, %edx			# edx = env->__regs[0]

	movw	0(%edx), %gs
	movw	4(%edx), %fs
	movw	8(%edx), %es
	movw	12(%edx), %ds

	movl	16(%edx), %edi
	movl	20(%edx), %esi
	movl	40(%edx), %ecx

	pushl	60(%edx)			# Restore eflags
	popfl
	movl	48(%edx), %ecx		# ecx = retAddr
	movl	24(%edx), %ebp		
	movl	28(%edx), %esp		
	movl	32(%edx), %ebx		
	movl	36(%edx), %edx
	jmpl	*%ecx



