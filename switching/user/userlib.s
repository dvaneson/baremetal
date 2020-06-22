	.text
	.globl	entry
entry:	leal	stack, %esp
	jmp	cmain

	.data
	.space	4096		# User stack
stack:

	# System call to print a character in the kernel's window
	.globl	kputc
kputc:	pushl	%eax
	mov	8(%esp), %eax
	int	$128
	popl	%eax
	ret

	# System call to check that hlt will not work for a user program
	.globl	yield
yield:	int $0x81
	ret

