/*
 *  linux/abi/abi_common/signal.c
 *
 *  This module does not go through the normal processing routines for
 *  ibcs. The reason for this is that for most events, the return is a
 *  procedure address for the previous setting. This procedure address
 *  may be negative which is not an error. Therefore, the return processing
 *  for standard functions is skipped by declaring this routine as a "special"
 *  module for the decoder and dealing with the register settings directly.
 *
 *  Please consider this closely if you plan on changing this mode.
 *  -- Al Longyear
 *
 * $Id$
 * $Source$
 */

#include <linux/config.h>

#define __NO_VERSION__
#include <linux/module.h>

#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/stddef.h>
#include <linux/unistd.h>
#include <linux/ptrace.h>
#include <linux/fcntl.h>
#include <linux/personality.h>
#include <linux/fs.h>
#include <linux/sys.h>
#include <linux/signal.h>

#include <asm/system.h>
#include <asm/uaccess.h>

#include <abi/abi.h>
#include <abi/xnx.h>
#include <abi/abi4.h>
#include <abi/map.h>


#define SIG_HOLD	((__sighandler_t)2)	/* hold signal */

#ifdef CONFIG_ABI_TRACE
#include <abi/trace.h>
#endif

#include <abi/signal.h>


typedef void (*pfn) (void);     /* Completion function */

/*
 *  Parameters to the signal functions have a common stack frame. This
 *  defines the stack frame.
 */

#define SIGNAL_NUMBER    get_syscall_parameter (regs, 0)
#define HIDDEN_PARAM     (SIGNAL_NUMBER & ~0xFF)
#define SECOND_PARAM     get_syscall_parameter (regs, 1)
#ifdef __sparc__
#define THIRD_PARAM      get_syscall_parameter (regs, 2)
#else /* __sparc__ */
#define THIRD_PARAM      ((unsigned long) regs->edx)
#endif /* __sparc__ */

/* Return a mask that includes SIG only.  */
#define __sigmask(sig)	(1 << ((sig) - 1))


#define TO_KERNEL(save)      \
	save = get_fs ();    \
	set_fs (get_ds ())

#define FROM_KERNEL(save)    \
	set_fs (save)

#define _S(nr) (1<<((nr)-1))

#define _BLOCKABLE (~(_S(IBCS_SIGKILL) | _S(IBCS_SIGSTOP)))

void
deactivate_signal(struct task_struct *task, int signum)
{
	spin_lock_irq(&task->sigmask_lock);
	sigdelset(&task->signal, signum);
	spin_unlock_irq(&task->sigmask_lock);
}


EXPORT_SYMBOL(deactivate_signal);

/*
 *  Translate the signal number to the corresponding item for Linux.
 */
static inline int abi_mapsig(int sig)
{
	if ((unsigned int) sig >= NSIGNALS)
		return -1;
	return current->exec_domain->signal_map[sig];
}


inline int abi_signo (struct pt_regs *regs, int *sig)
{
	int    value = abi_mapsig(SIGNAL_NUMBER & 0xFF);

	if (value == -1) {
		set_error (regs, iABI_errors (EINVAL));
		return 0;
	}

	*sig = value;
	return 1;
}

EXPORT_SYMBOL(abi_signo);

/*
 *  Process the signal() function from iBCS
 *
 *  This version appeared in "Advanced Programming in the Unix Environment"
 *  by W. Richard Stevens, page 298.
 */

void abi_sig_handler (struct pt_regs * regs, int sig,
			__sighandler_t handler, int oneshot)
{
	struct sigaction act, oact;
	int	      answer;
	mm_segment_t old_fs;

	sigemptyset (&act.sa_mask);
	act.sa_restorer = NULL;
	act.sa_handler = handler;
	act.sa_flags   = 0;

	if (oneshot)
		act.sa_flags = SA_ONESHOT | SA_NOMASK;
	else
		act.sa_flags = 0;

	TO_KERNEL (old_fs);
	answer = SYS(rt_sigaction) (sig, &act, &oact, sizeof(sigset_t));
	FROM_KERNEL (old_fs);

	if (answer < 0) {
		set_error (regs, iABI_errors (-answer));
	} else
		set_result (regs, (int) oact.sa_handler);
}

EXPORT_SYMBOL(abi_sig_handler);

/*
 *  Process the signal() function from iBCS
 */
int abi_signal (struct pt_regs * regs)
{
	__sighandler_t   vec;
	int	      sig;

	if (abi_signo (regs, &sig)) {
		vec = (__sighandler_t) SECOND_PARAM;
		abi_sig_handler (regs, sig, vec, 1);
	}
	return 0;
}

EXPORT_SYMBOL(abi_signal);

/*
 *      Process the iBCS sigset function.
 *
 *      This is basically the same as the signal() routine with the exception
 *      that it will accept a SIG_HOLD parameter.
 *
 *      A SIG_HOLD will defer the processing of the signal until a sigrelse()
 *      function is called.
 */
int abi_sigset (struct pt_regs * regs)
{
	sigset_t	 newmask, oldmask;
	__sighandler_t   vec;
	int	      sig, answer;
	mm_segment_t old_fs;

	if (abi_signo (regs, &sig)) {
		vec = (__sighandler_t) SECOND_PARAM;
		if (vec != SIG_HOLD) {
			deactivate_signal(current, sig);
			abi_sig_handler (regs, sig, vec, 0);
		} else {
/*
 *      Process the hold function
 */
			sigemptyset (&newmask);
			sigaddset  (&newmask, sig);

			TO_KERNEL (old_fs);
			answer = SYS(rt_sigprocmask) (SIG_BLOCK,
						&newmask, &oldmask,
						sizeof(sigset_t));
			FROM_KERNEL (old_fs);

			if (answer < 0) {
				set_error (regs, iABI_errors (-answer));
			}
		}
	}
	return 0;
}

EXPORT_SYMBOL(abi_sigset);

/*
 *      Process the iBCS sighold function.
 *
 *      Suspend the signal from future recognition.
 */
void abi_sighold (struct pt_regs * regs)
{
	sigset_t   newmask, oldmask;
	int	sig, answer;
	mm_segment_t old_fs;

	if (!abi_signo (regs, &sig))
		return;

	sigemptyset (&newmask);
	sigaddset  (&newmask, sig);

	TO_KERNEL (old_fs);
	answer = SYS(rt_sigprocmask) (SIG_BLOCK, &newmask, &oldmask,
				sizeof(sigset_t));
	FROM_KERNEL (old_fs);

	if (answer < 0) {
		set_error (regs, iABI_errors (-answer));
	}
}

EXPORT_SYMBOL(abi_sighold);

/*
 *      Process the iBCS sigrelse.
 *
 *      Re-enable the signal processing from a previously suspended
 *      signal. This may have been done by calling the sighold() function
 *      or a longjmp() during the signal processing routine. If you do a
 *      longjmp() function then it is expected that you will call sigrelse
 *      before going on with the program.
 */
void abi_sigrelse (struct pt_regs * regs)
{
	sigset_t   newmask, oldmask;
	int	sig, answer;
	mm_segment_t old_fs;

	if (!abi_signo (regs, &sig))
		return;

	sigemptyset (&newmask);
	sigaddset   (&newmask, sig);

	TO_KERNEL (old_fs);
	answer = SYS(rt_sigprocmask) (SIG_UNBLOCK, &newmask, &oldmask,
				sizeof(sigset_t));
	FROM_KERNEL (old_fs);

	if (answer < 0) {
		set_error (regs, iABI_errors (-answer));
	}
}

EXPORT_SYMBOL(abi_sigrelse);

/*
 *      Process the iBCS sigignore
 *
 *      This is basically a signal (...,SIG_IGN) call.
 */

void abi_sigignore (struct pt_regs * regs)
{
	struct sigaction act, oact;
	int	      sig, answer;
	mm_segment_t old_fs;

	if (!abi_signo (regs, &sig))
		return;

	sigemptyset (&act.sa_mask);

	act.sa_restorer = NULL;
	act.sa_handler = SIG_IGN;
	act.sa_flags   = 0;

	TO_KERNEL (old_fs);
	answer = SYS(rt_sigaction) (sig, &act, &oact, sizeof(sigset_t));
	FROM_KERNEL (old_fs);

	if (answer < 0) {
		set_error (regs, iABI_errors (-answer));
	}
}

EXPORT_SYMBOL(abi_sigignore);

/*
 *      Process the iBCS sigpause
 *
 *      Wait for the signal indicated to arrive before resuming the
 *      processing. I do not know if the signal is processed first using
 *      the normal event processing before the return. If someone can
 *      shed some light on this then please correct this code. I block
 *      the signal and look for it to show up in the pending list.
 */

void abi_sigpause (struct pt_regs * regs)
{
	old_sigset_t   newset;
	int	sig, answer;

#ifdef __sparc__
	printk(KERN_ERR "Sparc/iBCS: sigpause not yet implemented\n");
#else
	if (!abi_signo(regs, &sig))
		return;

	newset = ~0UL;
	newset &= (1UL << (sig-1));
	answer = SYS(sigsuspend)(0, current->blocked,
			newset, regs->esi, regs->edi,
			regs->ebp, regs->eax,
			regs->xds, regs->xes,
			regs->orig_eax,
			regs->eip, regs->xcs, regs->eflags,
			regs->esp, regs->xss);

	if (answer < 0) {
		set_error(regs, iABI_errors(-answer));
	}
#endif
}

EXPORT_SYMBOL(abi_sigpause);

/*
 *  This is the service routine for the syscall #48 (signal funcs).
 *
 *   Examine the request code and branch on the request to the appropriate
 *   function.
 */

int abi_sigfunc (struct pt_regs * regs)
{
	int sig_type = (int) HIDDEN_PARAM;

#ifdef CONFIG_ABI_TRACE
	if ((ibcs_trace & (TRACE_SIGNAL | TRACE_SIGNAL_F))
	|| ibcs_func_p->trace) {
		printk(KERN_DEBUG "iBCS2 sig%s(%ld, 0x%08lx, 0x%08lx)\n",
			sig_type == 0 ? "nal"
			: (sig_type == 0x100 ? "set"
			: (sig_type == 0x200 ? "hold"
			: (sig_type == 0x400 ? "relse"
			: (sig_type == 0x800 ? "ignore"
			: (sig_type == 0x1000 ? "pause"
			: "???" ))))),
			SIGNAL_NUMBER & 0xff, SECOND_PARAM, THIRD_PARAM);
	}
#endif

#ifdef __sparc__
	set_result (regs, 0);
#else /* __sparc__ */
	regs->eflags &= ~1;
	regs->eax     = 0;
#endif /* __sparc__ */
	switch (sig_type) {
	case 0x0000:
		abi_signal (regs);
		break;

	case 0x0100:
		abi_sigset (regs);
		break;

	case 0x0200:
		abi_sighold (regs);
		break;
		
	case 0x0400:
		abi_sigrelse (regs);
		break;

	case 0x0800:
		abi_sigignore (regs);
		break;

	case 0x1000:
		abi_sigpause (regs);
		break;

	default:
		set_error (regs, EINVAL);

#ifdef CONFIG_ABI_TRACE
		if ((ibcs_trace & (TRACE_SIGNAL | TRACE_SIGNAL_F))
		|| ibcs_func_p->trace)
		       printk (KERN_ERR "iBCS2 sigfunc(%x, %ld, %lx, %lx) unsupported\n",
			       sig_type,
			       SIGNAL_NUMBER,
			       SECOND_PARAM,
			       THIRD_PARAM);
#endif
		return 0;
	}

#ifdef CONFIG_ABI_TRACE
	if ((ibcs_trace & (TRACE_SIGNAL | TRACE_SIGNAL_F))
	|| ibcs_func_p->trace) {
		printk(KERN_DEBUG "iBCS2 returns %ld\n", get_result (regs));
	}
#endif
	return 0;
}

EXPORT_SYMBOL(abi_sigfunc);


/* This function is used to handle the sigaction call from SVr4 binaries.
   If anyone else uses this, this function needs to be modified since the
   order and size of the ibcs_sigaction structure is different in ibcs
   and the SVr4 ABI */


asmlinkage int abi_sigaction(int abi_signum, const struct abi_sigaction * action,
	struct abi_sigaction * oldaction)
{
	struct abi_sigaction new_sa, old_sa;
	int error, signo;
	mm_segment_t old_fs;
	struct sigaction nsa, osa;

	signo = abi_mapsig(abi_signum);
	if (signo == -1)
		return -EINVAL;

	if (oldaction) {
		error = verify_area(VERIFY_WRITE, oldaction,
				sizeof(struct abi_sigaction));
		if (error)
			return error;
	}

	if (action) {
		error = copy_from_user(&new_sa, action,
				sizeof(struct abi_sigaction));
		if (error)
			return -EFAULT;
		nsa.sa_restorer = NULL;
		nsa.sa_handler = new_sa.sa_handler;
		nsa.sa_mask = map_sigvec_to_kernel(new_sa.sa_mask,
			current->exec_domain->signal_map);
		if(new_sa.sa_flags & ABI_SA_ONSTACK)
			nsa.sa_flags |= SA_ONSTACK;
		if(new_sa.sa_flags & ABI_SA_RESTART)
			nsa.sa_flags |= SA_RESTART;
		if(new_sa.sa_flags & ABI_SA_NODEFER)
			nsa.sa_flags |= SA_NODEFER;
		if(new_sa.sa_flags & ABI_SA_RESETHAND)
			nsa.sa_flags |= SA_RESETHAND;
		if(new_sa.sa_flags & ABI_SA_NOCLDSTOP)
			nsa.sa_flags |= SA_NOCLDSTOP;
		if(new_sa.sa_flags & ABI_SA_NOCLDWAIT)
			nsa.sa_flags |= SA_NOCLDWAIT;
	}

	old_fs = get_fs();
	set_fs(get_ds());
	error = SYS(rt_sigaction)(signo,
				action ? &nsa : NULL,
				oldaction ? &osa : NULL,
				sizeof(sigset_t));
	set_fs(old_fs);

	if (!error && oldaction) {
		old_sa.sa_handler = osa.sa_handler;
		old_sa.sa_mask = map_sigvec_from_kernel(osa.sa_mask,
			current->exec_domain->signal_invmap);
		old_sa.sa_flags = 0;
		if(osa.sa_flags & SA_ONSTACK)
			old_sa.sa_flags |= ABI_SA_ONSTACK;
		if(osa.sa_flags & SA_RESTART)
			old_sa.sa_flags |= ABI_SA_RESTART;
		if(osa.sa_flags & SA_NODEFER)
			old_sa.sa_flags |= ABI_SA_NODEFER;
		if(osa.sa_flags & SA_RESETHAND)
			old_sa.sa_flags |= ABI_SA_RESETHAND;
		if(osa.sa_flags & SA_NOCLDSTOP)
			old_sa.sa_flags |= ABI_SA_NOCLDSTOP;
		if(osa.sa_flags & SA_NOCLDWAIT)
			old_sa.sa_flags |= ABI_SA_NOCLDWAIT;
		/* This should never fail... */
		copy_to_user(oldaction, &old_sa, sizeof(struct abi_sigaction));
	}
	return error;
}

EXPORT_SYMBOL(abi_sigaction);


static short int howcnv[] = {SIG_SETMASK, SIG_BLOCK, SIG_UNBLOCK, SIG_SETMASK};

asmlinkage int
abi_sigprocmask(int how, unsigned long *abinset, unsigned long *abioset)
{
	sigset_t new_set, *nset, old_set, *oset;
	unsigned long new_set_abi, old_set_abi;
	mm_segment_t old_fs;
	int error;

	nset = oset = NULL;

	if (abinset) {
		get_user(new_set_abi, abinset);
		new_set = map_sigvec_to_kernel(new_set_abi,
			current->exec_domain->signal_map);
		nset = &new_set;
	}
	if (abioset)
		oset = &old_set;

	old_fs = get_fs();
	set_fs(get_ds());
	error = SYS(rt_sigprocmask)(howcnv[how], nset, oset, sizeof(sigset_t));
	set_fs(old_fs);

	if (!error && abioset) {
		old_set_abi = map_sigvec_from_kernel(old_set,
			current->exec_domain->signal_invmap);
		put_user(old_set_abi, abioset);
	}

	return error;
}


EXPORT_SYMBOL(abi_sigprocmask);

#ifndef __sparc__
int abi_sigsuspend(struct pt_regs * regs)
{
	unsigned long * set;
	unsigned long oldset;
	old_sigset_t newset;
	int error;

	if (personality(PER_BSD)) {
		oldset = get_syscall_parameter (regs, 0);
	} else
	{
		set = (unsigned long *)get_syscall_parameter (regs, 0);
		error = get_user(oldset, set);
		if (error)
			return error;
	}
	newset = map_bitvec(oldset,
		current->exec_domain->signal_map);

#ifdef CONFIG_ABI_TRACE
	if ((ibcs_trace & TRACE_SIGNAL) || ibcs_func_p->trace)
		printk("iBCS: sigsuspend oldset, newset = %lx %lx\n",
			oldset, newset);
#endif
	{
#if 0
	    extern do_sigpause(unsigned int, struct pt_regs *);
	    return do_sigpause(newset, regs);
#endif
	}
	return SYS(sigsuspend)(0, oldset,
			newset, regs->esi, regs->edi,
			regs->ebp, regs->eax,
			regs->xds, regs->xes,
			regs->orig_eax,
			regs->eip, regs->xcs, regs->eflags,
			regs->esp, regs->xss);
}

EXPORT_SYMBOL(abi_sigsuspend);
#endif /* __sparc__ */
