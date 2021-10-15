	.text

# Disable hardware interrupts.
# void disableInterrupt();
	.globl	disableInterrupt
	.type	disableInterrupt, @function
disableInterrupt:
	cli
	retl


# Enable hardware interrupts.
# void enableInterrupt();
	.globl	enableInterrupt
	.type	enableInterrupt, @function
enableInterrupt:
	sti
	retl
