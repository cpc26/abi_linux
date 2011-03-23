/*
 *     arch/i386/abi/dispatch_i386.c
 *
 *  Copyright (C) 1993  Linus Torvalds
 *   Modified by Eric Youngdale to include all ibcs syscalls.
 *   Re-written by Drew Sullivan to handle lots more of the syscalls correctly.
 *
 *   Jan 30 1994, Merged Joe Portman's code -- Drew
 *   Jan 31 1994, Merged Eric Yongdale's code for elf support -- Drew
 *
 *   Feb 4 1994
 *     Rebuilt with handling for multiple binary personalities
 *     -- Mike Jagdis (jaggy@purplet.demon.co.uk)
 *
 *  Feb 14 1994
 *     Dual mode. Compiled in if you say yes to the configure iBCS
 *     question during 'make config'. Loadable module with kernel
 *     hooks otherwise.
 *     -- Mike Jagdis (jaggy@purplet.demon.co.uk)
 *
 *  Feb 18 1994
 *     Added the /dev/socksys emulator. This allows applications which
 *     use the socket interface to Lachman streams based TCP/IP to use
 *     the Linux TCP/IP stack.
 *     -- Mike Jagdis (jaggy@purplet.demon.co.uk)
 *
 * $Id$
 * $Source$
 */

#include <linux/config.h>
#define __NO_VERSION__
#include <linux/module.h>
#include <linux/version.h>

#include <asm/uaccess.h>

#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/stddef.h>
#include <linux/unistd.h>
#include <linux/ptrace.h>
#include <linux/fcntl.h>
#include <linux/init.h>

#include <asm/system.h>
#include <linux/fs.h>
#include <linux/sys.h>
#include <linux/personality.h>

#include <abi/abi.h>

#ifdef CONFIG_ABI_TRACE
#include <abi/trace.h>
static void plist(int id, char *name, char *args, int *list);
static void fail(int id, long eax, char *name);
int ibcs_trace = 0xffffffff;
EXPORT_SYMBOL(ibcs_trace);
int ibcs_id = 0;

static char *sig_names[] = {
	"SIGHUP",	"SIGINT",	"SIGQUIT",	"SIGILL",
	"SIGTRAP",	"SIGABRT/SIGIOT","SIGUNUSED",	"SIGFPE",
	"SIGKILL",	"SIGUSR1",	"SIGSEGV",	"SIGUSR2",
	"SIGPIPE",	"SIGALRM",	"SIGTERM",	"SIGSTKFLT",
	"SIGCHLD",	"SIGCONT",	"SIGSTOP",	"SIGTSTP",
	"SIGTTIN",	"SIGTTOU",	"SIGIO/SIGPOLL/SIGURG",
	"SIGXCPU",	"SIGXFSZ",	"SIGVTALRM",	"SIGPROF",
	"SIGWINCH",	"SIGLOST",	"SIGPWR",	"SIG 31",
	"SIG 32"
};

#ifdef CONFIG_ABI_VERBOSE_ERRORS
#  include "abi_errmap.h"
#endif /* CONFIG_ABI_VERBOSE_ERRORS */

#else /* CONFIG_ABI_TRACE */

static void fail(long eax);

#endif /* CONFIG_ABI_TRACE */


#define last(x)	((sizeof(x)/sizeof(*x))-1)


MODULE_AUTHOR("Mike Jagdis <jaggy@purplet.demon.co.uk>");
MODULE_DESCRIPTION("Support for non-Linux programs");
#ifdef CONFIG_ABI_TRACE
MODULE_PARM(ibcs_trace, "i");
MODULE_PARM_DESC(ibcs_trace, "iBCS debug trace");
#endif
//EXPORT_NO_SYMBOLS;

ABI_func * ibcs_func_p;
EXPORT_SYMBOL(ibcs_func_p);

void abi_dispatch(struct pt_regs *regs, ABI_func *p, int of)
{
	int	i;
	int	args[8];
	int	rvalue;
	void	*kfunc;
	short	nargs;
#ifdef CONFIG_ABI_TRACE
	int	id = ++ibcs_id;
#endif
	
	ibcs_func_p = p; 

	kfunc = p->kfunc;
	nargs = p->nargs;

	/* If the number of arguments is negative this is an unfudged
	 * system call function and we need to look up the real function
	 * address in the kernel's sys_call_table.
	 * Note that we never modify the callmap itself but do the lookup
	 * for each call. This allows modules that provide syscalls to
	 * be loaded and unloaded without any messy locking.
	 */
	if (nargs < 0) {
		kfunc = sys_call_table[(int)kfunc];

		/* Watch for a magic zero. This exists because we
		 * can't use -0 to represent a system call that
		 * takes no arguments.
		 */
		if (nargs == -ZERO)
			nargs = 0;
		else
			nargs = -nargs;
	}

	if (nargs <= (short)(sizeof(args)/sizeof(args[0])))
		for(i=0; i < nargs; i++)
			get_user(args[i], ((unsigned long *)regs->esp)+(i+of));

#ifdef CONFIG_ABI_TRACE
	if ((ibcs_trace & TRACE_API) || p->trace) {
		if (nargs == Spl) {
			for(i=0; i < (int)strlen(p->args); i++)
				get_user(args[i], ((unsigned long *) regs->esp) + (i+of));
		}
		plist(id, p->name, p->args, args);
	}
#endif

	rvalue = -ENOSYS;
	if (kfunc) {
		switch(nargs) {
		case Fast:
			((sysfun_p)kfunc)(regs);
#ifdef CONFIG_ABI_TRACEd
			if ((ibcs_trace & (TRACE_API|TRACE_SIGNAL))
			&& (signal_pending(current))) {
				unsigned long signr = current->signal.sig[0] & (~current->blocked.sig[0]);

				__asm__("bsf %1,%0\n\t"
					:"=r" (signr)
					:"0" (signr));
				printk(KERN_DEBUG "[%d]%d SIGNAL %lu <%s>\n",
					id, current->pid,
					signr+1, sig_namessignr]);
			}
#endif
			return;
		case Spl:
			rvalue = ((sysfun_p)kfunc)(regs);
			break;
		case 0:
			rvalue = ((sysfun_p)kfunc)();
			break;
		case 1:
			rvalue = ((sysfun_p)kfunc)(args[0]);
			break;
		case 2:
			rvalue = ((sysfun_p)kfunc)(args[0], args[1]);
			break;
		case 3:
			rvalue = ((sysfun_p)kfunc)(args[0], args[1], args[2]);
			break;
		case 4:
			rvalue = ((sysfun_p)kfunc)(args[0], args[1], args[2], args[3]);
			break;
		case 5:
			rvalue = ((sysfun_p)kfunc)(args[0], args[1], args[2],
					     args[3], args[4]);
			break;
		case 6:
			rvalue = ((sysfun_p)kfunc)(args[0], args[1], args[2],
					     args[3], args[4], args[5]);
			break;
		case 7:
			rvalue = ((sysfun_p)kfunc)(args[0], args[1], args[2],
					     args[3], args[4], args[5], 
						   args[6]);
			break;
		default:
#ifdef CONFIG_ABI_TRACE
			if ((ibcs_trace & TRACE_API) || p->trace)
				fail(id, regs->eax, p->name);
#else
			fail(regs->eax);
#endif
		}
	} else  {
#ifdef CONFIG_ABI_TRACE
		if ((ibcs_trace & TRACE_API) || p->trace)
			fail(id, regs->eax, p->name);
#else
		fail(regs->eax);
#endif
	}
	
	if (rvalue >= 0 || rvalue < -ENOIOCTLCMD) {
		regs->eflags &= ~1; /* Clear carry flag */
		regs->eax = rvalue;
#ifdef CONFIG_ABI_TRACE
		if ((ibcs_trace & TRACE_API) || p->trace) {
			printk(KERN_DEBUG "[%d]%d %s returns %ld {%ld}\n",
				id, current->pid, p->name,
				regs->eax, regs->edx);
		}
#endif
	} else {
		regs->eflags |= 1; /* Set carry flag */
		regs->eax = iABI_errors(-rvalue);
#ifdef CONFIG_ABI_TRACE
		if ((ibcs_trace & TRACE_API) || p->trace) {
			printk(KERN_DEBUG "[%d]%d %s error return "
#ifdef CONFIG_ABI_VERBOSE_ERRORS
				"linux=%d -> ibcs=%ld <%s>\n",
				id, current->pid, p->name,
				rvalue, regs->eax,
				-rvalue < (int)(sizeof(errmsg)/sizeof(errmsg[0]))
					? errmsg[-rvalue]
					: "unknown");
#else
				"linux=%d -> ibcs=%ld\n",
				id, current->pid, p->name,
				rvalue, regs->eax);
#endif
		}
#endif
	}
#ifdef CONFIG_ABI_TRACE
	if ((ibcs_trace & (TRACE_API|TRACE_SIGNAL))
	&& (signal_pending(current))) {
		unsigned long signr = current->signal.sig[0] & (~current->blocked.sig[0]);

		__asm__("bsf %1,%0\n\t"
			:"=r" (signr)
			:"0" (signr));
		printk(KERN_DEBUG "[%d]%d SIGNAL %lu <%s>, queued 0x%08lx\n",
			id, current->pid, signr+1, sig_names[signr],
			current->signal.sig[0]);
	}
#endif
}


EXPORT_SYMBOL(abi_dispatch);

int
abi_syscall(struct pt_regs *regs)
{
	get_user(regs->eax, ((unsigned long *) regs->esp) + 1);

	++regs->esp;
	current->exec_domain->handler(-1,regs);
	--regs->esp;

	return 0;
}

EXPORT_SYMBOL(abi_syscall);

#ifdef CONFIG_ABI_TRACE

/*
 * plist is used by the trace code to show the arg list
 */
static void plist(int id, char *name, char *args, int *list) {
	int	error;
	char	*tmp, *p, arg_buf[512];

	arg_buf[0] = '\0';
	p = arg_buf;
	while (*args) {
		switch(*args++) {
		case 'd': sprintf(p, "%d", *list++);		break;
		case 'o': sprintf(p, "0%o", *list++);		break;
		case 'p': sprintf(p, "0x%p", (void *)(*list++));	break;
		case '?': 
		case 'x': sprintf(p, "0x%x", *list++);		break;
		case 's': 
			tmp = getname((char *)(*list++));
			error = PTR_ERR(tmp);
			if (!IS_ERR(tmp)) {
				/* we are debugging, we don't need to see it all */
				tmp[80] = '\0';
				sprintf(p, "\"%s\"", tmp);
				putname(tmp);
			}
			break;
		default:
			sprintf(p, "?%c%c?", '%', args[-1]);
			break;
		}
		while (*p) ++p;
		if (*args) {
			*p++ = ',';
			*p++ = ' ';
			*p = '\0';
		}
	}
	printk(KERN_DEBUG "[%d]%d %s(%s)\n",
		id, current->pid, name, arg_buf);
}

static void fail(int id, long eax, char *name) {
	printk(KERN_ERR "[%d]%d Unsupported ABI function 0x%lx(%s)\n",
		id, current->pid, eax, name);
}
#else /* CONFIG_ABI_TRACE */
static void fail(long eax) {
	printk(KERN_ERR "Unsupported ABI function 0x%lx\n", eax);
}
#endif /* CONFIG_ABI_TRACE */
