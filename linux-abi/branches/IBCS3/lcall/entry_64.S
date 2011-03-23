#if _KSL < 14
#include <asm/offset.h>
#else
#include <asm/asm-offsets.h>
#endif

#include <asm/thread_info.h>

	.text
	.align	4,0x90
lcall7:
	swapgs
	movq	%rsp,%gs:pda_oldrsp 
	movq	%gs:pda_kernelstack,%rsp 
	movq	$7,(%rsp)
lcall:
	sti
	pushq	%rax
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rax
	pushq	%r8
	pushq	%r9
	pushq	%r10
	pushq	%r11
	pushq	%rbx
	pushq	%rbp
	pushq	%r12
	pushq	%r13
	pushq	%r14
	pushq	%r15

	movq	128(%rsp),%rdi		/* 7 or 27 */
	movq	136(%rsp),%rsi
	movq	%rsi,128(%rsp)		/* IP      */
	movq	144(%rsp),%rsi
	movq	%rsi,136(%rsp)		/* CS      */
	pushfq
	popq	%rsi
	movq 	%rsi,144(%rsp)		/* FLAGS   */
	movq	%rsp,%rsi

	GET_THREAD_INFO(%rdx)
	movq	8(%rdx),%rdx
	movq	8(%rdx),%rdx
	callq	*%rdx

	popq	%r15
	popq	%r14
	popq	%r13
	popq	%r12
	popq	%rbp
	popq	%rbx
	movq	32(%rsp),%rax

	pushq	sys_ret
	ret
lcall27:
	swapgs
	movq	%rsp,%gs:pda_oldrsp 
	movq	%gs:pda_kernelstack,%rsp 
	movq	$27,(%rsp)
	jmp	lcall
#-----------------------------------
	.align	4,0x90
	.globl	sys_call
sys_call:
	cmpq	$0,sys_tab
	je	_init_tab	
	cmpq	$512,%rdi
	je	_exit_tab	

	cmpq	$2,%rdi
	je	_forkexec
	cmpq	$11,%rdi
	je	_forkexec
	cmpq	$273,%rdi
	je	_vm86

	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	movq	64(%rsp),%rax
	pushq	%rax
	movq	%rdi,%rax
	movq	%rsi,%rdi
	movq	%rdx,%rsi
	movq	%rcx,%rdx
	movq	%r8,%rcx
	movq	%r9,%r8
	movq	64(%rsp),%r9
	salq	$3,%rax
	addq	sys_tab,%rax
	movq	(%rax),%rax
	callq	*%rax
	addq	$8,%rsp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	ret
_init_tab:
	pushq	%rbx
	pushq	%rcx
	pushq	%rdx
	
	movq	$512,%rcx
	movq	$-1,%rax
	movq	%rsi,%rbx
	movq	%rsi,exe_ret
	subq	$512,%rbx
_s1:
	inc	%rbx
	dec	%rcx
	jz	_init_ret
	cmpb	$255,(%rbx)
	jne	_s1
	cmpb	$20,1(%rbx)
	jne	_s1
	cmpb	$197,2(%rbx)
	jne	_s1

	movl	3(%rbx),%ecx
	movq	%rsi,%rax
	sarq	$32,%rax
	salq	$32,%rax
	orq	%rcx,%rax
	movq	%rax,sys_tab

	addq	$7,%rbx
	movq	%rbx,sys_ret
	
	movq	16(%rax),%rbx
	movslq	3(%rbx),%rcx
	addq	%rcx,%rbx
	addq	$7,%rbx
	movq	%rbx,_fork32

	movq	88(%rax),%rbx
	movslq	3(%rbx),%rcx
	addq	%rcx,%rbx
	addq	$7,%rbx
	movq	%rbx,_exec32

	movq	$sys_ldt,abi_ldt
	movq	abi_ldt,%rbx

	movq	$lcall7,%rdx
	movw	%cs,%ax
	sall	$16,%eax
	movw	%dx,%ax
	movw	$0x0EC00,%dx
	movl	%eax,(%rbx)
	movq	%rdx,4(%rbx)

	movq	$lcall27,%rdx
	movw	%cs,%ax
	sall	$16,%eax
	movw	%dx,%ax
	movw	$0x0EC00,%dx
	movl	%eax,32(%rbx)
	movq	%rdx,36(%rbx)

	movq	$0,%rax
_init_ret:
	popq	%rdx
	popq	%rcx
	popq	%rbx
	ret
_exit_tab:
	movq	$0,%rax
	ret
#------------------------------------
_vm86:
	subq	$8,%rsi
	movq	$abi_vm86,(%rsi)
	ret
#------------------------------
abi_vm86:
	cmpl	$0,%eax
	jl	abi_ret
	pushq	%rax
	callq	lcall_ldt
	popq	%rax
abi_ret:
	pushq	exe_ret
	ret
#-------------------------
_forkexec:
	pushq	%rcx
	pushq	%rdx
	pushq	%rsi
	pushq	%rdi

	movq	160(%rsi),%rax
	pushq	%rax
	movq	152(%rsi),%rax
	pushq	%rax
	movq	144(%rsi),%rax
	pushq	%rax
	movq	136(%rsi),%rax
	pushq	%rax
	movq	128(%rsi),%rax
	pushq	%rax
	movq	120(%rsi),%rax
	pushq	%rax
	movq	112(%rsi),%rax
	pushq	%rax
	movq	104(%rsi),%rax
	pushq	%rax
	movq	96(%rsi),%rax
	pushq	%rax
	movq	88(%rsi),%rax
	pushq	%rax
	movq	80(%rsi),%rax
	pushq	%rax
	movq	72(%rsi),%rax
	pushq	%rax
	movq	64(%rsi),%rax
	pushq	%rax
	movq	56(%rsi),%rax
	pushq	%rax
	movq	48(%rsi),%rax
	pushq	%rax
	movq	40(%rsi),%rax
	pushq	%rax
	movq	32(%rsi),%rax
	pushq	%rax
	movq	24(%rsi),%rax
	pushq	%rax
	movq	16(%rsi),%rax
	pushq	%rax
	movq	8(%rsi),%rax
	pushq	%rax
	movq	(%rsi),%rax
	pushq	%rax

	cmpq	$2,%rdi
	je	_do_fork
_do_exec:
	movq	%rsp,%rcx	
	movl	96(%rsi),%edx
	movl	40(%rsi),%edi
	movl	88(%rsi),%esi
	movq	_exec32,%rax
	jmp	_do_fe
_do_fork:
	movq	%rsp,%rdi
	movq	_fork32,%rax
	jmp	_do_fe
_do_fe:
	callq	*%rax
	movq	%rax,%rdi
	movq	176(%rsp),%rsi

	popq	%rax
	movq	%rax,(%rsi)
	popq	%rax
	movq	%rax,8(%rsi)	
	popq	%rax
	movq	%rax,16(%rsi)	
	popq	%rax
	movq	%rax,24(%rsi)	
	popq	%rax
	movq	%rax,32(%rsi)	
	popq	%rax
	movq	%rax,40(%rsi)	
	popq	%rax
	movq	%rax,48(%rsi)	
	popq	%rax
	movq	%rax,56(%rsi)	
	popq	%rax
	movq	%rax,64(%rsi)	
	popq	%rax
	movq	%rax,72(%rsi)	
	popq	%rax
	movq	%rax,80(%rsi)	
	popq	%rax
	movq	%rax,88(%rsi)	
	popq	%rax
	movq	%rax,96(%rsi)	
	popq	%rax
	movq	%rax,104(%rsi)	
	popq	%rax
	movq	%rax,112(%rsi)	
	popq	%rax
	movq	%rax,120(%rsi)	
	popq	%rax
	movq	%rax,128(%rsi)	
	popq	%rax
	movq	%rax,136(%rsi)	
	popq	%rax
	movq	%rax,144(%rsi)	
	popq	%rax
	movq	%rax,152(%rsi)	
	popq	%rax
	movq	%rax,160(%rsi)	

	movq	%rdi,%rax
	popq	%rdi
	popq	%rsi
	popq	%rdx
	popq	%rcx
	ret
#-------------------------
	.data
	.globl	sys_ldt
sys_ldt:
	.zero	48
sys_tab:
	.zero	8
sys_ret:
	.zero	8
exe_ret:
	.zero	8
_fork32:
	.zero	8
_exec32:
	.zero	8

