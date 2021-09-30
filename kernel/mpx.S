#include "minix/config.h"
#include "minix/com.h"
#include "protect.h"	

	.text
# Set up a C stack frame on the monitor stack.
	pushl	%esp
	movl	%ebp, %esp
	pushl	%esi
	pushl	%edi

# Copy the monitor GDT to the address space of kernel and 
# switch over to it. 
	sgdt	gdt+GDT_SELECTOR			# Get the monitor GDTR (GDT Register)
	movl	gdt+GDT_SELECTOR+2, %esi	# Absolute address of GDT
	movl	$gdt, %ebx					# Address of kernel GDT
	movl	$(8*8), %ecx					# Copy 8 descriptors
.copyGDT:
	movb	%es:(%esi), %al
	movb	%al, (%ebx)
	incl	%esi
	incl	%ebx
	loop	.copyGDT
	
aa:
	jmp aa
