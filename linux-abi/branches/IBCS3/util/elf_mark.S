	.text
	.globl _start
_start:
	popl	%eax		// argc
	cmpl	$2,%eax
	jle	_usage
	popl	%ecx		// arg0
	popl	%ecx		// arg1
	cmpb	$45,(%ecx)
	jne	_usage

	cmpb	$48,1(%ecx)	// Linux
	je	_do_files

	movl	$894587727,MARK	// "OSR5"
	cmpb	$53,1(%ecx)
	je	_do_files

	movl	$827016277,MARK	// "UDK1"
	cmpb	$55,1(%ecx)
	je	_do_files

	movl	$877811283,MARK	// "SVR4"
	cmpb	$52,1(%ecx)
	je	_do_files

	movl	$909652568,MARK	// "X286"
	cmpb	$50,1(%ecx)
	je	_do_files

	jmp	_usage

_do_files:
	popl	%ecx
	cmpl	$0,%ecx
	je	_exit

	call	std_out
	movl	%ecx,%ebx
	movl	$2,%ecx
	movl	$5,%eax
	int	$128
	cmpl	$0,%eax
	jg	_read
	movl	$F_OP,%ecx
	call	std_out
	jmp	_do_files
_read:
	movl	%eax,FD
	movl	$40,%edx
	movl	$E_H,%ecx
	movl	FD,%ebx
	movl	$3,%eax		//read
	int	$128
	cmpl	$40,%eax
	je	_check
	movl	$F_RD,%ecx
	call	std_out
	jmp	_close
_check:
	cmpl	$1179403647,E_H	// ".ELF"
	je	_mark
	movl	$F_HD,%ecx
	call	std_out
	jmp	_close
_mark:
	movl	MARK,%eax
	movl	%eax,E_H+36
	movl	$0,%edx
	movl	$0,%ecx
	movl	FD,%ebx
	movl	$19,%eax	// lseek
	int	$128
	cmpl	$0,%eax
	je	_write
	movl	$F_LS,%ecx
	call	std_out
	jmp	_close
_write:
	movl	$40,%edx
	movl	$E_H,%ecx
	movl	FD,%ebx
	movl	$4,%eax		// write
	int	$128
	cmpl	$40,%eax
	je	_tell
	movl	$F_WR,%ecx
	call	std_out
	jmp	_close
_tell:
	movl	$F_OK,%ecx
	call	std_out
_close:
	movl	FD,%ebx
	movl	$6,%eax
	int	$128
	xorl	%ebx,%ebx
	jmp	_do_files
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
MARK:	.long	0
FD:	.long	0
E_H:	.zero	40
Use:	.string	"Usage: "
Parm:	.string	" -{0|2|4|5|7} files... (sets Linux, X286, SVR4, OSR5 or UW7 mark on ELF binaries)\n"
F_OP:	.string	" - unable to open\n"
F_RD:	.string	" - read error\n"
F_WR:	.string	" - write error\n"
F_HD:	.string	" - bad header\n"
F_LS:	.string	" - lseek error\n"
F_OK:	.string	" - marked OK\n"
