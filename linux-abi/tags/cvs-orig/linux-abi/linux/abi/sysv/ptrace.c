/*
 *  linux/abi/svr4_common/ptrace.c
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

#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/user.h>

#include <abi/abi.h>
#include <abi/signal.h>

#ifdef CONFIG_ABI_TRACE
#include <abi/trace.h>
#endif


#define NREGS	19

#define U(X) ((unsigned long)&((struct user *)0)->X)

#ifdef CONFIG_ABI_IBCS_SCO
static unsigned long sco_to_linux_reg[NREGS] = {
	U(regs.gs),	U(regs.fs),	U(regs.es),	U(regs.ds),
	U(regs.edi),	U(regs.esi),
	U(regs.ebp),
	U(regs.esp	/* ESP */),
	U(regs.ebx),	U(regs.edx),	U(regs.ecx),	U(regs.eax),
	U(signal	/* Trap */),
	U(reserved	/* ERR */),
	U(regs.eip),	U(regs.cs),	U(regs.eflags),
	U(regs.esp),	U(regs.ss)
};
#endif

#ifdef CONFIG_ABI_IBCS_WYSE
static unsigned long wysev386_to_linux_reg[NREGS] = {
	U(regs.es),	U(regs.ds),	U(regs.edi),	U(regs.esi),
	U(regs.ebp),	U(regs.esp),
	U(regs.ebx),	U(regs.edx),	U(regs.ecx),	U(regs.eax),
	U(signal	/* Trap */),
	U(reserved	/* ERR */),
	U(regs.eip),	U(regs.cs),
	U(regs.eflags),
	U(regs.esp	/* UESP */),
	U(regs.ss),
	U(regs.fs),	U(regs.gs)
};
#endif


unsigned long *reg_map[] = {
	NULL,
	NULL,			/* SVR4 */
	NULL,			/* SVR3 is a subset of SVR4 */
#ifdef CONFIG_ABI_IBCS_SCO
	sco_to_linux_reg,	/* SCO SVR3 */
#else
	NULL,
#endif
#ifdef CONFIG_ABI_IBCS_WYSE
	wysev386_to_linux_reg,	/* Wyse V/386 */
#else
	NULL,
#endif
	NULL,			/* ISC R4 */
	NULL,			/* BSD */
	NULL			/* Xenix */
};


int svr4_ptrace(int req, int pid, unsigned long addr, unsigned long data)
{
#if !defined(CONFIG_ABI_IBCS_SCO) && !defined(CONFIG_ABI_IBCS_WYSE)
	return -EIO;
#else
	unsigned long res;

	/* Slight variations between iBCS and Linux codes. */
	if (req == PTRACE_ATTACH)
		req = 10;
	else if (req == PTRACE_DETACH)
		req = 11;

	/* Remap access to the registers. */
	if (req == 3 || req == 6) {
		if (addr == 0x1200	/* get offset of u_ar0 (SCO) */
		|| addr == 0x1292) {	/* get offset of u_ar0 (Wyse V/386) */
			return 0x4000;
		}

		if ((addr & 0xff00) == 0x4000) { /* Registers */
			addr = (addr & 0xff) >> 2;
			if (addr > NREGS
			|| (int)(addr = reg_map[current->personality & PER_MASK][addr]) == -1)
				return -EIO;
		}
	}

	if (req == 7 && data > 0) {
		if (data > NSIGNALS)
			return -EIO;
		data = current->exec_domain->signal_map[data];
	}

	if (req == 1 || req == 2 || req == 3) {
		mm_segment_t old_fs;
		int error;

		old_fs = get_fs();
		set_fs(get_ds());
		error = SYS(ptrace)(req, pid, addr, &res);
		set_fs(old_fs);
		if (error)
			return error;
	}

#ifdef CONFIG_ABI_TRACE
	if (((ibcs_trace & TRACE_API) || ibcs_func_p->trace)
	&& (req == 3 || req == 6)) {
		static char *regnam[] = { "EBX", "ECX", "EDX",
			"ESI", "EDI", "EBP", "EAX", "DS", "ES",
			"FS", "GS", "ORIG_EAX", "EIP", "CS", "EFL",
			"UESP", "SS"
		};
		printk(KERN_DEBUG "iBCS: %ld [%s] = 0x%08lx\n",
			addr>>2,
			(addr>>2) < sizeof(regnam)/sizeof(regnam[0])
				? regnam[addr>>2] : "???",
			req == 3 ? res : data);
	}
#endif
	if (req == 1 || req == 2 || req == 3)
		return res;

	return SYS(ptrace)(req, pid, addr, data);
#endif /* !defined(CONFIG_ABI_IBCS_SCO) && !defined(CONFIG_ABI_IBCS_WYSE) */
}

EXPORT_SYMBOL(svr4_ptrace);
