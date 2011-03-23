/*
 *  linux/ibcs/bsdsignal.c
 *
 *  Copyright (C) 1994  Mike Jagdis (jaggy@purplet.demon.co.uk)
 *
 * $Id$
 * $Source$
 */

#include <linux/config.h>

#include <linux/version.h>

#include <asm/uaccess.h>

#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/stddef.h>
#include <linux/unistd.h>
#include <linux/ptrace.h>
#include <linux/config.h>
#include <linux/fcntl.h>
#include <linux/personality.h>

#include <asm/system.h>
#include <linux/fs.h>
#include <linux/sys.h>
#include <linux/signal.h>

#include <abi/signal.h>

#include <abi/map.h>
#include <abi/bsd.h>
#include <abi/abi.h>


#define _S(nr) (1<<((nr)-1))

#define _BLOCKABLE (~(_S(BSD_SIGKILL) | _S(BSD_SIGSTOP)))


int
bsd_sigaction(int bsd_signum, const struct bsd_sigaction *action,
	struct bsd_sigaction *oldaction)
{
	struct bsd_sigaction new_sa, old_sa;
	int error, signo;
	mm_segment_t old_fs;
	struct sigaction nsa, osa;

	signo = current->exec_domain->signal_map[bsd_signum];
	if (signo == -1)
		return -EINVAL;

	if (oldaction) {
		error = verify_area(VERIFY_WRITE, oldaction,
				sizeof(struct bsd_sigaction));
		if (error)
			return error;
	}

	if (action) {
		error = copy_from_user(&new_sa, action,
				sizeof(struct bsd_sigaction));
		if (error)
			return -EFAULT;
		nsa.sa_restorer = NULL;
	  	nsa.sa_handler = new_sa.sa_handler;
		nsa.sa_mask = map_sigvec_to_kernel(new_sa.sa_mask,
			current->exec_domain->signal_map);
		nsa.sa_flags = 0;
		if (new_sa.sa_flags & BSD_SA_NOCLDSTOP)
			nsa.sa_flags |= SA_NOCLDSTOP;
		if (new_sa.sa_flags & BSD_SA_ONSTACK)
			nsa.sa_flags |= SA_ONSTACK;
		if (new_sa.sa_flags & BSD_SA_RESTART)
			nsa.sa_flags |= SA_RESTART;
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
		if(osa.sa_flags & SA_NOCLDSTOP)
			old_sa.sa_flags |= BSD_SA_NOCLDSTOP;
		if(osa.sa_flags & SA_ONSTACK)
			old_sa.sa_flags |= BSD_SA_ONSTACK;
		if(osa.sa_flags & SA_RESTART)
			old_sa.sa_flags |= BSD_SA_RESTART;
		/* This should never fail... */
		copy_to_user(oldaction, &old_sa, sizeof(struct bsd_sigaction));
	}
	return error;
}


/* BSD passes the pointer to the new set to the library function but
 * replaces it with the actual signal set before handing off to the
 * syscall. Although the pointer to the old set is still in the stack
 * frame during the syscall the syscall returns the old set in eax
 * and the library code does the save if necessary.
 */
int
bsd_sigprocmask(int how, unsigned long bsdnset, unsigned long *bsdoset)
{
	unsigned long old_set;
	sigset_t new_set;

	old_set = map_sigvec_from_kernel(current->blocked,
			current->exec_domain->signal_invmap),

	new_set = map_sigvec_to_kernel(bsdnset,
		current->exec_domain->signal_map);

	spin_lock_irq(&current->sigmask_lock);
	switch (how) {
		case 1: /* SIGBLOCK */
			sigorsets(&current->blocked,
				&current->blocked, &new_set);
			break;
		case 2: /* SIGUNBLOCK */
			signotset(&new_set);
			sigandsets(&current->blocked,
				&current->blocked, &new_set);
			break;
		case 3: /* SIGSETMASK */
			current->blocked = new_set;
			break;
		default:
			spin_unlock_irq(&current->sigmask_lock);
			return -EINVAL;
	}

	spin_unlock_irq(&current->sigmask_lock);
	return old_set;
}


/* Although the stack frame contains the pointer to where the set should
 * be stored BSD returns the set in eax and the library code does the
 * store.
 */
int
bsd_sigpending(unsigned long *set)
{
	sigset_t mask;

	spin_lock_irq(&current->sigmask_lock);
	sigandsets(&mask, &current->blocked, &current->signal);
	spin_unlock_irq(&current->sigmask_lock);
	return map_sigvec_from_kernel(mask,
			current->exec_domain->signal_invmap);
}
