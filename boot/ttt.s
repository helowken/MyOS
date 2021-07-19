	.code16gcc
	.text
movw %cs, %ax
movw %ax, %ds
movw %ax, %es
cld

pushl $0x4
calll main2
addl $0x4, %esp

pushl $0x5
calll main2
addl $0x4, %esp

pushl $0x6
calll main2
addl $0x4, %esp


hlt:
	jmp hlt

.global display
display:
	pushl %ebp
	movw 0x8(%esp), %bp
	movw 0xc(%esp), %cx
	movb 0x10(%esp), %dh

	movw $0x1301, %ax
	movw $0x000c, %bx
	movb $0x0, %dl
	int $0x10
	popl %ebp
	retl

.global print
print:
	pushl %ebp
	movb $0x0E, %ah
	movb 0x8(%esp), %al
	int $0x10
	movb 0x9(%esp), %al
	int $0x10
	movb 0xa(%esp), %al
	int $0x10
	movb 0xb(%esp), %al
	int $0x10
	popl %ebp
	retl
	
