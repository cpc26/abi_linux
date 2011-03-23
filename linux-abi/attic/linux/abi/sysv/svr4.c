/*
 *  linux/abi/svr4_common/svr4.c
 *
 *  Copyright (C) 1995  Mike Jagdis
 *
 * $Id$
 * $Source$
 */

#include <linux/config.h>
#define __NO_VERSION__
#include <linux/module.h>
#include <linux/version.h>

#include <asm/uaccess.h>

#include <linux/types.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/stddef.h>
#include <linux/unistd.h>
#include <linux/ptrace.h>
#include <linux/fcntl.h>
#include <linux/time.h>

#include <asm/system.h>
#include <linux/fs.h>
#include <linux/sys.h>
#include <linux/malloc.h>

#include <abi/abi.h>
#include <abi/abi4.h>
#include <abi/svr4.h>

#ifdef CONFIG_ABI_TRACE
#include <abi/trace.h>
#endif

#include <abi/svr4sig.h>
#include <abi/solaris.h>

/* Interactive SVR4's /bin/sh calls access(... 011) but Linux returns
 * EINVAL if the access mode has any other bits than 007 set.
 */
int
svr4_access(char *path, int mode)
{
	return SYS(access)(path, mode & 007);
}


int svr4_getgroups(int n, unsigned long *buf)
{
	int i;

	if (n) {
		i = verify_area(VERIFY_WRITE, buf, sizeof(unsigned long) * n);
		if (i)
			return i;
	}
	for (i = 0; i < current->ngroups && i < n; i++) {
		put_user(current->groups[i], buf);
		buf++;
	}
	return(i);
}


int svr4_setgroups(int n, unsigned long *buf)
{
	int i;

	if (!capable(CAP_SETGID))
		return -EPERM;
	if (n > NGROUPS)
		return -EINVAL;
	/* FIXME: Yuk! What if we hit a bad address? */
	for (i = 0; i < n; i++, buf++) {
		get_user(current->groups[i], buf);
	}
	current->ngroups = n;
	return 0;
}


int svr4_waitid(int idtype, int id, struct svr4_siginfo *infop, int options)
{
	long result, kopt;
	mm_segment_t old_fs;
	int pid, status;

	switch (idtype) {
		case 0: /* P_PID */
			pid = id;
			break;

		case 1: /* P_PGID */
			pid = -id;
			break;

		case 7: /* P_ALL */
			pid = -1;
			break;

		default:
			return -EINVAL;
	}

	if (infop) {
		result = verify_area(VERIFY_WRITE, infop,
					sizeof(struct svr4_siginfo));
		if (result)
			return result;
	}

	kopt = 0;
	if (options & 0100) kopt |= WNOHANG;
	if (options & 4) kopt |= WUNTRACED;

	old_fs = get_fs();
	set_fs(get_ds());
	result = SYS(wait4)(pid, &status, kopt, NULL);
	set_fs(old_fs);
	if (result < 0)
		return result;

	if (infop) {
		unsigned long op, st;

		put_user(current->exec_domain->signal_map[SIGCHLD],
			&infop->si_signo);
		put_user(result,
			&infop->_data._proc._pid);

		if ((status & 0xff) == 0) {
			/* Normal exit. */
			op = SVR4_CLD_EXITED;
			st = status >> 8;
		} else if ((status & 0xff) == 0x7f) {
			/* Stopped. */
			st = (status & 0xff00) >> 8;
			op = (st == SIGSTOP || st == SIGTSTP)
				? SVR4_CLD_STOPPED
				: SVR4_CLD_CONTINUED;
			st = current->exec_domain->signal_invmap[st];
		} else {
			st = (status & 0xff00) >> 8;
			op = (status & 0200)
				? SVR4_CLD_DUMPED
				: SVR4_CLD_KILLED;
			st = current->exec_domain->signal_invmap[st];
		}
		put_user(op, &infop->si_code);
		put_user(st, &infop->_data._proc._pdata._cld._status);
	}
	return 0;
}

EXPORT_SYMBOL(svr4_waitid);

int svr4_seteuid(int uid)
{
	return SYS(setreuid)(-1, uid);
}

int svr4_setegid(int gid)
{
	return SYS(setregid)(-1, gid);
}

int svr4_pathconf(char *path, int name)
{
	switch (name) {
		case _PC_LINK_MAX:
			/* Although Linux headers define values on a per
			 * filesystem basis there is no way to access
			 * these without hard coding fs information here
			 * so for now we use a bogus value.
			 */
			return LINK_MAX;

		case _PC_MAX_CANON:
			return MAX_CANON;

		case _PC_MAX_INPUT:
			return MAX_INPUT;

		case _PC_PATH_MAX:
			return PATH_MAX;

		case _PC_PIPE_BUF:
			return PIPE_BUF;

		case _PC_CHOWN_RESTRICTED:
			/* We should really think about this and tell
			 * the truth.
			 */
			return 0;

		case _PC_NO_TRUNC:
			/* Not sure... It could be fs dependent? */
			return 1;

		case _PC_VDISABLE:
			return 1;

		case _PC_NAME_MAX: {
			struct statfs buf;
			char *p;
			int error;
			mm_segment_t old_fs;

			p = getname(path);
			error = PTR_ERR(p);
			if (!IS_ERR(p)) {
				old_fs = get_fs();
				set_fs (get_ds());
				error = SYS(statfs)(p, &buf);
				set_fs(old_fs);
				putname(p);
				if (!error)
					return buf.f_namelen;
			}
			return error;
		}
	}

	return -EINVAL;
}

EXPORT_SYMBOL(svr4_pathconf);

int svr4_fpathconf(int fildes, int name)
{
	switch (name) {
		case _PC_LINK_MAX:
			/* Although Linux headers define values on a per
			 * filesystem basis there is no way to access
			 * these without hard coding fs information here
			 * so for now we use a bogus value.
			 */
			return LINK_MAX;

		case _PC_MAX_CANON:
			return MAX_CANON;

		case _PC_MAX_INPUT:
			return MAX_INPUT;

		case _PC_PATH_MAX:
			return PATH_MAX;

		case _PC_PIPE_BUF:
			return PIPE_BUF;

		case _PC_CHOWN_RESTRICTED:
			/* We should really think about this and tell
			 * the truth.
			 */
			return 0;

		case _PC_NO_TRUNC:
			/* Not sure... It could be fs dependent? */
			return 1;

		case _PC_VDISABLE:
			return 1;

		case _PC_NAME_MAX: {
			struct statfs buf;
			int error;
			mm_segment_t old_fs;

			old_fs = get_fs();
			set_fs (get_ds());
			error = SYS(statfs)(fildes, &buf);
			set_fs(old_fs);
			if (!error)
				return buf.f_namelen;
			return error;
		}
	}

	return -EINVAL;
}

EXPORT_SYMBOL(svr4_fpathconf);

int svr4_sigpending(int which_routine, svr4_sigset_t *set)
{
	/* Solaris multiplexes on this one */
	/* Which routine has the actual routine that should be called */

	switch (which_routine){
	case 1:			/* sigpending */
		printk ("iBCS/Intel: sigpending not implemented\n");
		return -EINVAL;
		
	case 2:			/* sigfillset */
		set->setbits [0] = ~0;
		set->setbits [1] = 0;
		set->setbits [2] = 0;
		set->setbits [3] = 0;
		return 0;
	}
	return -EINVAL;
}

EXPORT_SYMBOL(svr4_sigpending);

static int svr4_setcontext(svr4_ucontext_t *c, struct pt_regs *regs)
{
	printk ("Getting context\n");
	return 0;
}

static int svr4_getcontext(svr4_ucontext_t *c, struct pt_regs *regs)
{
	printk ("Setting context\n");
	return 0;
}

int svr4_context(struct pt_regs *regs)
{
	int context_fn = get_syscall_parameter (regs, 0);
	struct svr4_ucontext_t *uc = (void *) get_syscall_parameter (regs, 1);
   
	switch (context_fn){
	case 0: /* getcontext */
		return svr4_getcontext (uc, regs);

	case 1: /* setcontext */
		return svr4_setcontext (uc, regs);
	}
	return -EINVAL;
}

EXPORT_SYMBOL(svr4_context);
