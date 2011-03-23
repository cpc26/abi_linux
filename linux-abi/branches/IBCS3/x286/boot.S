		.text
/* Routine called by x286 program to do a syscall */
		.align 4
.globl __x286syscall
__x286syscall:
		fs/movl %esp,__em_tesp	/* Save stack pointer */
		fs/mov  %ss,__em_tss	/* Save stack segment reg */
		fs/lss __em_esp,%esp	/* Load emulator stack pointer */
		pushfl
		fs/addl $8,__em_tesp	/* remove the return loc. from the prog stack */
		popfl
/* now make the stack look like a trap was hit */
		fs/pushl __em_tss
		fs/pushl __em_tesp
		pushfl
		pushl $0		/* Space for cs */
		pushl $0  		/* Space for eip */
		pushl $0		/* err */
		pushl $0		/* trapno */
		pushl %eax
		pushl %ecx
		pushl %edx
		pushl %ebx
		fs/pushl __em_tesp
		pushl %ebp
		pushl %esi
		pushl %edi
		pushl %ds
		pushl %es
		pushl %fs
		pushl %gs
		fs/mov __em_ss,%ds	/* assume ss = ds = es in 386 prog ? */
		fs/mov __em_ss,%es
		mov %esp,%ebp
		call _x286syscall
		popl %gs
		popl %fs
		popl %es
		popl %ds
		popl %edi
		popl %esi
		popl %ebp
		fs/popl __em_tesp
		popl %ebx
		popl %edx
		popl %ecx
		popl %eax
		addl $8,%esp		/* err and trapno */
		addl $8,%esp		/* eip and cs */
		fs/subl $4,__em_tesp	/* put return loc. back */
		popfl
		fs/popl __em_tss	/* get rid of esp at signal */
		fs/popl __em_tss
		fs/lss __em_tesp,%esp	/* back to 286 stack */
		.byte 0x66
		lret
		

/* lcall7(function,...) */
/* returns result in eax and poss edx */
		.align 4
.globl lcall7
lcall7:
		popl %edx
		popl %eax
		pushl %edx
		movl %eax,%ebx
		.byte 0x9a,0,0,0,0,7,0	/* lcall $7,0 */
		jnc L1
		movl %eax,xerrno
		movl $-1,%eax
L1:
		popl %ecx
		pushl %ebx
		pushl %ecx
		ret

		.align 8
.globl xigret
xigret:
		popl %eax
		movl $0x77,%eax
		int  $0x80

/* Set the initial execution state and enter the 286 program. */
		.align 4
.globl x286boot
x286boot:
		movl %esp,__em_esp	/* save emulator stack */
		mov  %ss,__em_ss
		subl %eax,%eax
		mov  %ds,%ax		/* ensure %fs == %ds */
		mov  %ax,%fs
		subl %eax,%eax
		movl %eax,%ebx
		movl %eax,%ecx
		movl %eax,%edx
		movl %eax,%ebp
		movl %eax,%esi
		movl %eax,%edi
		push %eax
		popfl

		lss init_stk,%esp
		movl %esp,%ebp

		movl init_ds,%eax
		mov  %ax,%ds
		mov  %ax,%es

		fs/ljmp *init_entry

		.data
		.align 4
		.globl init_cs
		.globl init_entry
		.globl init_ds
		.globl limit_stk
		.globl init_ss
		.globl init_stk
init_entry:	.long 0
init_cs:	.long 0
limit_stk:	.long 0
init_ds:	.long 0
init_stk:	.long 0
init_ss:	.long 0

__em_esp:	.long 0
__em_ss:	.long 0
__em_tesp:	.long 0
__em_tss:	.long 0
