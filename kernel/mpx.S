#include "minix/config.h"
#include "minix/com.h"
#include "const.h"
#include "protect.h"	

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
	movw	%ax, %ss
	movl	$kStackTop, %esp			# Set sp to point to the top of kernel stack


aa:
	jmp aa



	.section	.bss
	.lcomm kStack, K_STACK_BYTES		# Kernel stack
kStackTop:								# Top of kernel stack

