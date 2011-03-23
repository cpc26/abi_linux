	.text
	.globl _start
_start:
	cmpl	$1,(%esp)
	jne  	_exit
	cmpl	$0,12(%esp)
	je  	_exit

	movl	4(%esp),%ebx
	movl	$0,%edx
	movl	%esp,%ecx
	addl	$4,%ecx
	
	movl	$11,%eax
	int	$128
	movl	%eax,%ebx
_exit:
	movl	$1,%eax
	int 	$128
