	.code16gcc
	.text
movw	%cs, %ax
movw	%ax, %ds
movw	%ax, %es		# Set es = ds = cs
cld						

# Clear bss
xorb	%al, %al
movw	edata, %di
movw	end, %cx
subw	%di, %cx
rep
stosb


xorw	%ax, %ax
movw	%cs, %dx
callw	seg2Abs
leaw	caddr, %bx
movw	%ax, (%bx)
movw	%dx, 2(%bx)


calll	test
calll	boot





halt:
	jmp halt


#========== Print Functions ==========
	.globl	kputc
	.type	kputc, @function
kputc:							# Called by C printf()
	movb	4(%esp), %al		# al = char 
	testb	%al, %al			# if al == 0, do nothing.
	jz	.noChar
	movb	$0xe, %ah			# Call BIOS ah=0xE
	cmpb	$0xA, %al			# if al = 'LF' (new line)
	jnz	.putChar
	movb	$0xD, %al			# then output 'CR' + 'LF'
	int	$0x10
	movb	$0xA, %al
.putChar:
	int	$0x10
.noChar:
	retl

	.globl	print
	.type	print, @function
print:
	movl	4(%esp), %ecx		# ecx = address of string
.printLoop:
	movb	(%ecx), %al			# al = char of string
	testb	%al, %al			# if al == 0 => ends of string
	je	.printEnd
	movb	$0xe, %ah			# Call BIOS ah=0xE
	int	$0x10
	inc	%ecx					# Go to the next char
	jmp	.printLoop
.printEnd:
	retl

	.globl	println
	.type	println, @function
println:
	pushl	%ebp
	movl	%esp, %ebp
	movl	0x8(%ebp), %eax
	pushl	%eax
	calll	print				# Call print
	movb	$0xe, %ah			
	movb	$0xd, %al			# Output 'CR'
	int	$0x10
	movb	$0xa, %al			# Output 'LF'
	int	$0x10
	leave
	retl

#========== Copy Functions ==========
	.globl	rawCopy
	.type	rawCopy, @function
rawCopy:
	pushl	%ebp
	movl	%esp, %ebp
	pushw	%si
	pushw	%di
.copy:
	cmpl	$0, 18(%ebp)		# high 16 bits of runSize
	jnz	.bigCopy

.bigCopy:
	movw	$0xFFF0, %cx
	leave
	retl

#========== Addr Functions ==========
	.globl	derefSp
	.type	derefSp, @function
derefSp:								# Get value from SS instead of DS. Value is got from DS by default.
	movl	4(%esp), %eax
	movl	%ss:(%eax), %eax
	retl

	.type	abs2Seg, @function
abs2Seg:								# Transfer the 32 bit address dx-ax to dx:ax
	pushw	%cx
	movb	%dl, %ch
	movw	%ax, %dx
	andw	$0x000F, %ax
	movb	$4, %cl
	shrw	%cl, %dx
	shlb	%cl, %ch
	orb	%ch, %dh
	popw	%cx
	retw

	.type seg2Abs, @function
seg2Abs:
	pushw	%cx
	movb	%dh, %ch
	movb	$4, %cl
	shlw	%cl, %dx
	shrb	%cl, %ch
	addw	%dx, %ax
	adcb	$0, %ch
	movb	%ch, %dl
	xorb	%dh, %dh
	popw	%cx
	retw

#========== Detect Memory Functions ==========
	.globl	detectLowMem
	.type	detectLowMem, @function
detectLowMem:
	pushl	%ebp
	movl	%esp, %ebp
	clc						# Clear carry flag
	xor	%eax, %eax
	int	$0x12			# Switch to the BIOS (= request low memory size)
	jnc	.detectEnd
	jmp	.detectError

	.globl	detect88Mem
	.type	detect88Mem, @function
detect88Mem:
	pushl	%ebp
	movl	%esp, %ebp
	clc
	xor	%eax, %eax
	movb	$0x88, %ah
	int		$0x15
	jc	.detectError
	testw	%ax, %ax
	je	.detectError
	jmp	.detectEnd

	.globl	detectE801Mem
	.type	detectE801Mem, @function
detectE801Mem:
	pushl	%ebp
	movl	%esp, %ebp
	xor	%ecx, %ecx
	xor %edx, %edx
	mov $0xE801, %ax
	int $0x15
	jc	.detectError
	cmpb	$0x86, %ah
	je	.detectError
	cmpb	$0x80, %ah
	je	.detectError
	testw	%cx, %cx
	jz .detectError
	movl	16(%ebp), %ebx
	movl	8(%ebp), %eax
	testl	%ebx, %ebx
	jz	.useDS
	movl	%ecx, %ss:(%eax)
	movl	12(%ebp), %eax
	movl	%edx, %ss:(%eax)
	jmp .E801End
.useDS:
	movl	%ecx, (%eax)
	movl	12(%ebp), %eax
	movl	%edx, (%eax)
.E801End:
	movl	$0, %eax
	jmp .detectEnd

	.globl	detectE820Mem
	.type	detectE820Mem, @function
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
	pushl	$.detectErrMsg
	calll	println
	addl	$4, %esp
	movl	$-1, %eax
.detectEnd:
	leave
	retl

	.section	.rodata
.detectErrMsg:
	.string	"Detect Memory Failed."
