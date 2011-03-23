	.text
	.globl _start
_start:
	popl	%eax		// argc
	cmpl	$2,%eax
	jne	_usage
	xorl	%edx,%edx
	popl	%ebx		// arg0
	popl	%ebx		// arg1
_get_lev:
	movb	(%ebx),%al
	cmpb	$48,%al
	jl	_do_trace
	cmpb	$57,%al
	jg	_do_trace
	movl	%edx,%ecx
	sall	$2,%ecx
	addl	%ecx,%edx
	addl	%edx,%edx
	subl	$48,%eax
	addl	%eax,%edx
	inc	%ebx
	jmp	_get_lev
_do_trace:
	movl	%edx,%eax
	addl	$65280,%eax
	lcall	$7,$0
	movl	%eax,%ebx
	jmp	_exit
_usage:
	movl	$Use,%ecx
	call	std_out
	popl	%ecx		// arg0
	call	std_out
	movl	$Parm,%ecx
	call	std_out
	movl	$139,%ebx
_exit:
	movl	$1,%eax
	int 	$128
std_out:
	xorl	%edx,%edx
	movl	%ecx,%ebx
_get_len:
	cmpb	$0,(%ebx)
	je	_do_out
	inc	%edx
	inc	%ebx
	jmp	_get_len
_do_out:
	movl	$1,%ebx
	movl	$4,%eax
	int	$128
	ret

	.data
Use:	.string	"Usage: "
Parm:	.string	" trace_level_from_0_to_255\n"
