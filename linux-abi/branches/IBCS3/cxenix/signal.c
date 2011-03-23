//#ident "%W% %G%"

#define __KERNEL_SYSCALLS__
#include "../include/util/i386_std.h"
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/personality.h>
#include <linux/sched.h>
#include <asm/uaccess.h>

#include "../include/util/trace.h"
#include "../include/cxenix/signal.h"
#include "../include/signal.h"

#include "../include/util/map.h"
#include "../include/util/sysent.h"

unsigned long abi_sigret(int);

int
xnx_sigaction(int sco_signum, const struct sco_sigaction *action,
		struct sco_sigaction *oldaction)
{
	struct sco_sigaction	new_sa, old_sa;
	struct sigaction	nsa, osa;
	mm_segment_t		fs;
	int			error, signo;

	if (sco_signum >= NSIGNALS)
		return -EINVAL;
	signo = current_thread_info()->exec_domain->signal_map[sco_signum];

	if (oldaction) {
		if (!access_ok(VERIFY_WRITE, oldaction,
				sizeof(struct sco_sigaction)))
			return -EFAULT;
	}

	if (action) {
		error = copy_from_user(&new_sa, action,
				sizeof(struct sco_sigaction));
		if (error)
			return -EFAULT;
		nsa.sa_handler = new_sa.sa_handler;
		nsa.sa_mask = map_sigvec_to_kernel(new_sa.sa_mask,
			current_thread_info()->exec_domain->signal_map);
		nsa.sa_flags = SA_NOMASK | SA_SIGINFO;
		if (new_sa.sa_flags & SCO_SA_NOCLDSTOP)
			nsa.sa_flags |= SA_NOCLDSTOP;
		nsa.sa_restorer = (void *) abi_sigret(__NR_rt_sigaction);
		if(nsa.sa_restorer) nsa.sa_flags |= SA_RESTORER;
	}

	fs = get_fs();
	set_fs(get_ds());
	error = SYS(rt_sigaction,signo, action ? &nsa : NULL,
			oldaction ? &osa : NULL, sizeof(sigset_t));
	set_fs(fs);

	if (error || !oldaction)
		return (error);

	old_sa.sa_handler = osa.sa_handler;
	old_sa.sa_mask = map_sigvec_from_kernel(osa.sa_mask,
			current_thread_info()->exec_domain->signal_invmap);
	old_sa.sa_flags = 0;
	if (osa.sa_flags & SA_NOCLDSTOP)
		old_sa.sa_flags |= SCO_SA_NOCLDSTOP;

	if (copy_to_user(oldaction, &old_sa, sizeof(struct sco_sigaction)))
		return -EFAULT;
	return 0;
}

int
xnx_sigpending(u_long *setp)
{
	sigset_t		lxpending;
	u_long			pending;

	spin_lock_irq(&current->sighand->siglock);
	sigandsets(&lxpending, &current->blocked, &current->pending.signal);
	spin_unlock_irq(&current->sighand->siglock);

	pending = map_sigvec_from_kernel(lxpending,
			current_thread_info()->exec_domain->signal_invmap);

	if (copy_to_user(setp, &pending, sizeof(u_long)))
		return -EFAULT;
	return 0;
}
