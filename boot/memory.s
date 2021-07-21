	.code16gcc
	.text
	.globl	detectLowMem
	.type	text, @function
detectLowMem:
	pushl	%ebp
	movl	%esp, %ebp
	clc						# Clear carry flag
	xor	%eax, %eax
	int	$0x12			# Switch to the BIOS (= request low memory size)
	jnc	.detectEnd
	jmp	.detectError

	.globl	detectE820Mem
	.type	text, @function
detectE820Mem:	
	pushl	%ebp
	movl	%esp, %ebp
	subl	$4, %esp
	movw	destE820, %di	
	pushl	%ebx
	xor	%ebx, %ebx
	movl	$0, -4(%ebp)
	movl	$0x534D4150, %edx
	movl	$0xe820, %eax
	movw	$1, 20(%di)
	movl	$24, %ecx
	int	$0x15
	jc	.detectError
	movl	$0x534D4150, %edx
	cmpl	%edx, %eax
	jne	.detectError
	testl	%ebx, %ebx
	je	.detectError
	jmp	.E820Check
.E820Loop:
	movl	$0xe820, %eax
	movw	$1, 20(%di)
	movl	$24, %ecx
	int	$0x15
	jc	.E820End
	movl	$0x534D4150, %edx
.E820Check:
	jcxz	.skipEntry
	cmpb	$20, %cl
	jbe	.notExtendEntry
	testb	$1, 20(%di)
	je	.skipEntry
.notExtendEntry:
	movl	8(%di), %ecx
	orl	12(%di), %ecx
	jz	.skipEntry
	incb	-4(%ebp)
	addw	$24, %di
.skipEntry:
	testl	%ebx, %ebx
	jne	.E820Loop
.E820End:
	clc
	movl	-4(%ebp), %eax			# Return entry count.
	popl	%ebx
	jmp	.detectEnd

.detectError:
	pushl	$.errMsg
	calll	println
	addl	$4, %esp
	movl	$-1, %eax
.detectEnd:
	leave
	retl


	.section	.rodata
.errMsg:
	.string	"Detect Memory Failed."
