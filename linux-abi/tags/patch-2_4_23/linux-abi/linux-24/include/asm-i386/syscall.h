#ifndef _ASM_SYSCALL_H
#define _ASM_SYSCALL_H

/*
 * Prototypes for architecture-specific Linux syscalls.
 */


/* arch/i386/kernel/ldt.c */
extern asmlinkage int	sys_modify_ldt(int, void *, unsigned long);

/* arch/i386/kernel/process.c */
extern asmlinkage int	sys_fork(struct pt_regs regs);                                                      

/* arch/i386/kernel/ptrace.c */
extern asmlinkage int	sys_ptrace(long request, long pid,
				long addr, long data);

/* arch/i386/kernel/signal.c */
extern int		sigsuspend1(struct pt_regs *regs, old_sigset_t mask);

/* arch/i386/kernel/sys_i386.c */
extern asmlinkage int	sys_ipc(uint call, int first, int second,
				int third, void *ptr, long fifth);
extern asmlinkage int	sys_pause(void);
extern asmlinkage int	sys_pipe(unsigned long * fildes);

#endif /* _ASM_SYSCALL_H */
