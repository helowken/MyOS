.equ K_STACK_BYTES, 1024

	.text
jmp	main

	.section	.bss
	.lcomm	kStack, K_STACK_BYTES	/* Kernel stack */
