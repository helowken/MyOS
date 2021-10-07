#include "minix/config.h"
#include "minix/com.h"
#include "ibm/interrupt.h"
#include "const.h"
#include "protect.h"	
.include "sconst.inc"

	.text
# Set up a C stack frame on the monitor stack.
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%esi
	pushl	%edi

# Copy the monitor GDT to the address space of kernel 
# and switch over to it. 
	sgdt	gdt+GDT_SELECTOR			# Get the monitor GDTR (GDT Register)
	movl	gdt+GDT_SELECTOR+2, %esi	# Absolute address of GDT
	movl	$gdt, %ebx					# Address of kernel GDT
	movl	$(8*8), %ecx				# Copy 8 descriptors
.copyGDT:
	movb	%es:(%esi), %al
	movb	%al, (%ebx)
	incl	%esi
	incl	%ebx
	loop	.copyGDT
	
	movl	gdt+DS_SELECTOR+2, %eax		# Base of kernel data
	andl	$0xFFFFFF, %eax				# Only 24 bits
	addl	$gdt, %eax					# Convert gdt address from virtual to physical
	movl	%eax, gdt+GDT_SELECTOR+2	# Set base of GDT
	lgdt	gdt+GDT_SELECTOR			# Switch over to kernel GDT

# Locate boot parameters, set up kernel segment registers and stack.
	movl	8(%ebp), %ebx				# Boot parameters offset (ss relative)
	movl	12(%ebp), %edx				# Boot parameters length
	movl	16(%ebp), %eax				# Address of image headers
	movl	%eax, imgHdrPos				
	movw	%ds, %ax					# Kernel data selector
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs
	movw	%ax, %ss					# ss = gs = fs = es = ds
	movl	$kStackTop, %esp			# Set sp to point to the top of kernel stack

# Call C startup code to set up a proper environment to run main().	
	pushl	%edx
	pushl	%ebx
	pushl	SS_SELECTOR
	pushl	DS_SELECTOR
	pushl	CS_SELECTOR
	calll	cstart						# cstart(cs, ds, ss, paramOffset, paramSize)
	addl	$(5*4), %esp

aa:
	jmp aa


#*=====================================================================*
#*                Exception Handlers                *
#*=====================================================================*
	.globl	divideError
	.type	divideError, @function
divideError:
	pushl	DIVIDE_VECTOR
	jmp	exception


	.globl	singleStepException
	.type	singleStepException, @function
singleStepException:
	pushl	DEBUG_VECTOR
	jmp exception


	.globl	nmi
	.type	nmi, @function
nmi:
	pushl	NMI_VECTOR
	jmp exception


	.globl	breakpointException
	.type	breakpointException, @function
breakpointException:
	pushl	BREAKPOINT_VECTOR
	jmp	exception


	.globl	overflow
	.type	overflow, @function
overflow:
	pushl	OVERFLOW_VECTOR
	jmp	exception


	.globl	boundsCheck
	.type	boundsCheck, @function
boundsCheck:
	pushl	BOUNDS_VECTOR
	jmp	exception


	.globl	invalOpcode
	.type	invalOpcode, @function
invalOpcode:
	pushl	INVAL_OP_VECTOR
	jmp exception


	.globl	coprNotAvailable
	.type	coprNotAvailable, @function
coprNotAvailable:
	pushl	COPROC_NOT_VECTOR
	jmp	exception


	.globl	doubleFault
	.type	doubleFault, @function
doubleFault:
	pushl	DOUBLE_FAULT_VECTOR
	jmp	errException


	.globl	coprSegOverrun
	.type	coprSegOverrun, @function
coprSegOverrun:
	pushl	COPROC_SEG_VECTOR
	jmp exception


	.globl	invalTss
	.type	invalTss, @function
invalTss:
	pushl	INVAL_TSS_VECTOR
	jmp	errException


	.globl	segmentNotPresent
	.type	segmentNotPresent, @function
segmentNotPresent:
	pushl	SEG_NOT_VECTOR
	jmp errException


	.globl	stackException
	.type	stackException, @function
stackException:
	pushl	STACK_FAULT_VECTOR
	jmp errException


	.globl	generalProtection
	.type	generalProtection, @function
generalProtection:
	pushl	PROTECTION_VECTOR
	jmp errException


	.globl	pageFault
	.type	pageFault, @function
pageFault:
	pushl	PAGE_FAULT_VECTOR
	jmp errException


	.globl	coprError
	.type	coprError, @function
coprError:
	pushl	COPROC_ERR_VECTOR
	jmp	exception


#*=====================================================================*
#*                exception                *
#*=====================================================================*
	.type	exception, @function
	.align	16
exception:
	movl	$0, %ss:(errorCode)			# Clear errorCode
	popl	%ss:(exVector)
	jmp	.commonException	
	

#*=====================================================================*
#*                errException                *
#*=====================================================================*
	.type	errException, @function
	.align	16
errException:
	popl	%ss:(errorCode)			
	popl	%ss:(exVector)
.commonException:						# Common for all exceptions
	pushl	%eax
	movl	4(%esp), %eax				# Save old EIP
	movl	%eax, %ss:(oldEip)
	movl	8(%esp), %eax				# Save old CS
	movl	%eax, %ss:(oldCs)
	movl	12(%esp), %eax				# Save old EFLAGS
	movl	%eax, %ss:(oldEflags)
	popl	%eax
	calll	save
	pushl	oldEflags
	pushl	oldCs
	pushl	oldEip
	pushl	errorCode
	pushl	exVector
	calll	handleException				# handleException(exVector, errorCode, oldEip, oldCs, oldEflags);
	addl	$(5*4), %esp
	retl


#*=====================================================================*
#*                save                *
#*=====================================================================*
# Save for protected mode.
	.type	save, @function
	.align	16
save:
	cld									
	pushal								# Save all general registers
	pushw	%ds
	pushw	%es
	pushw	%fs
	pushw	%gs							# Save ds, es, fs, gs
	movw	%ss, %dx					# ss is kernel data segment (See above)
	movw	%dx, %ds					# Load rest of kernel segments
	movw	%dx, %es					# Kernel does not use fs, gs
	movl	%esp, %eax
	incb	kernelReentryCount
	jnz	.setRestart						# Stack is already kernel stack
.setupKernelStack:						# TODO
	jmp .setupKernelStack

	.align	4
.setRestart:
	pushl	.doRestart
	addl	$(RETADR-P_STACKBASE), %eax
	jmp	%eax


#*=====================================================================*
#*                restart                *
#*=====================================================================*
.doRestart:
	decb	kernelReentryCount
	popw	%gs
	popw	%fs
	popw	%es
	popw	%ds
	popal
	addl	$4, %esp					# Skip return address
	iretl								# Continue process






	.section	.bss
	.lcomm	kStack, K_STACK_BYTES		# Kernel stack
kStackTop:								# Top of kernel stack
	.lcomm	exVector, 4	
	.lcomm	errorCode, 4
	.lcomm	oldEip, 4
	.lcomm	oldCs, 4
	.lcomm	oldEflags, 4
