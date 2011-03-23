#include <asm/segment.h>
#include <asm/thread_info.h>

EBX	=  0
ECX	=  4
EDX	=  8
ESI	= 12
EDI	= 16
EBP	= 20
EAX	= 24
DS	= 28
ES	= 32
#if         _KSL > 19
FS	= 36
#if         _KSL > 29
GS	= 40
OAX	= 44
EIP	= 48
CS	= 52
EFL	= 56
OSP	= 60
OSS	= 64
#else
OAX	= 40
EIP	= 44
CS	= 48
EFL	= 52
OSP	= 56
OSS	= 60
#endif
#else
OAX	= 36
EIP	= 40
CS	= 44
EFL	= 48
OSP	= 52
OSS	= 56
#endif

	.text
	.align	4,0x90
lcall7:
	pushfl
	pushl	%eax
	movl	$0x7,%eax
do_lcall:
	cld
#if _KSL > 19
#if _KSL > 20
#if _KSL > 29
	pushl	%gs
#endif
	pushl	%fs
#else
	pushl	%gs
#endif
#endif
	pushl 	%es
	pushl 	%ds
	pushl 	%eax
	pushl 	%ebp
	pushl 	%edi
	pushl 	%esi
	pushl 	%edx
	pushl 	%ecx
	pushl 	%ebx
	movl 	$(__USER_DS),%edx
	movl 	%edx,%ds
	movl 	%edx,%es
#if _KSL > 19
#if _KSL > 21
	movl 	$(__KERNEL_PERCPU),%edx
#else
	movl 	$(__KERNEL_PDA),%edx
#endif
#if _KSL > 20
	movl 	%edx,%fs
#else
	movl 	%edx,%gs
#endif
#endif
#if _KSL > 29	
		# must be based on "CONFIG_X86_32_LAZY_GS" instead !!!
	movl	$(__KERNEL_STACK_CANARY),%edx
	movl 	%edx,%gs
#endif
	movl	%esp,%ebp
	pushl	%ebp
	pushl	%eax		# 0x7 or 0x27
	movl	OAX(%ebp),%eax	# restore original eax
	movl	%eax,EAX(%ebp)

	movl	EIP(%ebp),%eax	# due to call gates, this is eflags, not eip..
	movl	CS(%ebp),%edx	# this is eip..
	movl	EFL(%ebp),%ecx	# and this is cs..
	movl	%eax,EFL(%ebp)	#
	movl	%edx,EIP(%ebp)	# Now we move them to their "normal" places
	movl	%ecx,CS(%ebp)	#

	andl	$-THREAD_SIZE,%ebp
	movl	4(%ebp),%edx	# Get Execution Domain
	call	*4(%edx)	# Lcall handler for the Domain

	addl	$8,%esp
	movl	EAX(%esp),%eax
	movl	%eax,OAX(%esp)

	pushl	sys_ret
	ret

	popl	%ebx
	popl	%ecx
	popl	%edx
	popl	%esi
	popl	%edi
	popl	%ebp
	popl	%eax
	pop	%ds
	pop	%es
#if _KSL > 19
#if _KSL > 20
	pop	%fs
#if _KSL > 29
	pop	%gs
#endif
#else
	pop	%gs
#endif
#endif
	addl	$4,%esp
	iret
#------------------------------
	.align	4,0x90
lcall27:
	pushfl
	pushl	%eax
	movl	$0x27,%eax
	jmp	do_lcall
#------------------------------
abi_vm86:
	cmpl	$0,%eax
	jl	abi_ret
	pushl	%eax
	call	lcall_ldt
	popl	%eax
abi_ret:
	pushl	sys_ret
	ret
#------------------------------
	.globl	sys_call
sys_call:
	cmpl	$0,sys_tab
	je	_init_tab
	cmpl	$512,4(%esp)
	je	_exit_tab

	cmpl	$2,4(%esp)
	je	_forkexec
	cmpl	$11,4(%esp)
	je	_forkexec
	cmpl	$273,4(%esp)
	je	_vm86

	movl	32(%esp),%eax
	pushl	%eax
	movl	32(%esp),%eax
	pushl	%eax
	movl	32(%esp),%eax
	pushl	%eax
	movl	32(%esp),%eax
	pushl	%eax
	movl	32(%esp),%eax
	pushl	%eax
	movl	32(%esp),%eax
	pushl	%eax
	movl	32(%esp),%eax
	pushl	%eax
				# we accept 7 parameters at most
	movl	32(%esp),%eax
	sall	$2,%eax
	addl	sys_tab,%eax
	movl	(%eax),%eax
	call	*%eax

	addl	$28,%esp
	ret
#------------------------------------
_init_tab:
	pushl	%ebx
	pushl	%ecx
	pushl	%edx

	movl	20(%esp),%eax
	cmpl	$0,%eax
	je	_idt
	movl	%eax,sys_ret
	subl	$4,%eax
	movl	(%eax),%ebx
	movl	%ebx,sys_tab
	jmp	_bs
_idt:
	sidt	sys_xdt
	movl	sys_xdt+2,%ebx
	addl	$1024,%ebx
	movzwl	(%ebx),%edx
	movzwl	6(%ebx),%eax
	sall	$16,%eax
	orl	%edx,%eax
_bs:
	movl	$512,%ecx
	movl	%eax,%ebx
	movl	$-1,%eax
_s:
	incl	%ebx	
	decl	%ecx
	jz	_ldt
	cmpb	$255,(%ebx)
	jne	_s
	cmpb	$20,1(%ebx)
	jne	_s
	cmpb	$133,2(%ebx)
	jne	_s

	movl	3(%ebx),%eax
	movl	%eax,sys_tab
	addl	$7,%ebx
	movl	%ebx,sys_ret
_ldt:
	cmpl	$0,sys_tab
	je	_init_ret

	movl	$sys_ldt,abi_ldt
	movl	abi_ldt,%ebx

	movl	$lcall7,%edx
	movw	%cs,%ax
	sall	$16,%eax
	movw	%dx,%ax
	movw	$0x0EC00,%dx
	movl	%eax,(%ebx)
	movl	%edx,4(%ebx)

	movl	$lcall27,%edx
	movw	%cs,%ax
	sall	$16,%eax
	movw	%dx,%ax
	movw	$0x0EC00,%dx
	movl	%eax,32(%ebx)
	movl	%edx,36(%ebx)

	movl	$0,%eax
_init_ret:
	popl	%edx
	popl	%ecx
	popl	%ebx
	ret
#------------------------------------
_exit_tab:
	movl	$0,%eax
	movl	%eax,sys_tab	
	ret
#------------------------------------
_vm86:
	movl	8(%esp),%eax
	sub	$4,%eax
	cmpl	$_fe_ret,(%eax)
	je	_vm_ret
	movl	$abi_vm86,(%eax)
_vm_ret:
	ret
#------------------------------------
_forkexec:
	pushl	%ebp
	movl	12(%esp),%ebp
	
	movl	64(%ebp),%eax
	pushl	%eax
	movl	60(%ebp),%eax
	pushl	%eax
	movl	56(%ebp),%eax
	pushl	%eax
	movl	52(%ebp),%eax
	pushl	%eax
	movl	48(%ebp),%eax
	pushl	%eax
	movl	44(%ebp),%eax
	pushl	%eax
	movl	40(%ebp),%eax
	pushl	%eax
	movl	36(%ebp),%eax
	pushl	%eax
	movl	32(%ebp),%eax
	pushl	%eax
	movl	28(%ebp),%eax
	pushl	%eax
	movl	24(%ebp),%eax
	pushl	%eax
	movl	20(%ebp),%eax
	pushl	%eax
	movl	16(%ebp),%eax
	pushl	%eax
	movl	12(%ebp),%eax
	pushl	%eax
	movl	8(%ebp),%eax
	pushl	%eax
	movl	4(%ebp),%eax
	pushl	%eax
	movl	(%ebp),%eax
	pushl	%eax

	movl	76(%esp),%eax
	sall	$2,%eax
	addl	sys_tab,%eax
	movl	(%eax),%eax
	call	*%eax
_fe_ret:
	movl	%eax,76(%esp)

	movl	(%esp),%eax
	movl	%eax,(%ebp)
	movl	4(%esp),%eax
	movl	%eax,4(%ebp)
	movl	8(%esp),%eax
	movl	%eax,8(%ebp)
	movl	12(%esp),%eax
	movl	%eax,12(%ebp)
	movl	16(%esp),%eax
	movl	%eax,16(%ebp)
	movl	20(%esp),%eax
	movl	%eax,20(%ebp)
	movl	24(%esp),%eax
	movl	%eax,24(%ebp)
	movl	28(%esp),%eax
	movl	%eax,28(%ebp)
	movl	32(%esp),%eax
	movl	%eax,32(%ebp)
	movl	36(%esp),%eax
	movl	%eax,36(%ebp)
	movl	40(%esp),%eax
	movl	%eax,40(%ebp)
	movl	44(%esp),%eax
	movl	%eax,44(%ebp)
	movl	48(%esp),%eax
	movl	%eax,48(%ebp)
	movl	52(%esp),%eax
	movl	%eax,52(%ebp)
	movl	56(%esp),%eax
	movl	%eax,56(%ebp)
	movl	60(%esp),%eax
	movl	%eax,60(%ebp)
	movl	64(%esp),%eax
	movl	%eax,64(%ebp)
	
	movl	76(%esp),%eax
	addl	$68,%esp
	sub	$4,%ebp
	movl	$abi_vm86,(%ebp)
	popl	%ebp
	ret
#------------------------------------
	.data
sys_ldt:
	.zero	48
sys_tab:
	.zero	4
sys_ret:
	.zero	4
sys_xdt:
	.zero	8
