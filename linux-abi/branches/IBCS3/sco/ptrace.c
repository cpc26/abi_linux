/*
 * ptrace.c - SCO OpenServer ptrace(2) support.
 *
 * Copyright (c) 1995 Mike Jagdis (jaggy@purplet.demon.co.uk)
 */

//#ident "%W% %G%"

/*
 * This file is nearly identical to abi/wyse/ptrace.c, please keep it in sync.
 */
#define __KERNEL_SYSCALLS__
#include "../include/util/i386_std.h"
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/personality.h>
#include <linux/user.h>
//#include <linux/unistd.h>
#include <linux/ptrace.h>
#include <linux/syscalls.h>

#include <asm/uaccess.h>

#include "../include/signal.h"
#include "../include/util/trace.h"


#define NREGS	19
#define U(X)	((unsigned long)&((struct user *)0)->X)

#ifdef CONFIG_64BIT
#if _KSL > 24
static unsigned long sco_to_linux_reg[NREGS] = {
	U(regs.gs),	U(regs.fs),	U(regs.es),	U(regs.ds),
	U(regs.di),	U(regs.si),
	U(regs.bp),
	U(regs.sp	/* ESP */),
	U(regs.bx),	U(regs.dx),	U(regs.cx),	U(regs.ax),
	U(signal	/* Trap */),
	U(reserved	/* ERR */),
	U(regs.ip),	U(regs.cs),	U(regs.flags),
	U(regs.sp),	U(regs.ss)
};
#else
static unsigned long sco_to_linux_reg[NREGS] = {
	U(regs.gs),	U(regs.fs),	U(regs.es),	U(regs.ds),
	U(regs.rdi),	U(regs.rsi),
	U(regs.rbp),
	U(regs.rsp	/* ESP */),
	U(regs.rbx),	U(regs.rdx),	U(regs.rcx),	U(regs.rax),
	U(signal	/* Trap */),
	U(reserved	/* ERR */),
	U(regs.rip),	U(regs.cs),	U(regs.eflags),
	U(regs.rsp),	U(regs.ss)
};
#endif
#else
#if _KSL > 24
static unsigned long sco_to_linux_reg[NREGS] = {
	U(regs.gs),	U(regs.fs),	U(regs.es),	U(regs.ds),
	U(regs.di),	U(regs.si),
	U(regs.bp),
	U(regs.sp	/* ESP */),
	U(regs.bx),	U(regs.dx),	U(regs.cx),	U(regs.ax),
	U(signal	/* Trap */),
	U(reserved	/* ERR */),
	U(regs.ip),	U(regs.cs),	U(regs.flags),
	U(regs.sp),	U(regs.ss)
};
#else
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
#endif

#if defined(CONFIG_ABI_TRACE)
static const char *regnam[] = {
	"EBX", "ECX", "EDX", "ESI", "EDI", "EBP", "EAX",
	"DS", "ES", "FS", "GS", "ORIG_EAX", "EIP", "CS",
	"EFL", "UESP", "SS"
};
#endif

int
sco_ptrace(int req, int pid, u_long addr, u_long data)
{
	u_long			res;

	/*
	 * Slight variations between iBCS and Linux codes.
	 */
	if (req == PTRACE_ATTACH)
		req = 10;
	else if (req == PTRACE_DETACH)
		req = 11;

	if (req == 3 || req == 6) {
		/* get offset of u_ar0 */
		if (addr == 0x1200)
			return 0x4000;

		/* remap access to the registers. */
		if ((addr & 0xff00) == 0x4000) { /* Registers */
			addr = (addr & 0xff) >> 2;
			if (addr > NREGS)
				return -EIO;
			addr = sco_to_linux_reg[addr];
			if (addr == -1)
				return -EIO;
		}
	}

	if (req == 7 && data > 0) {
		if (data > NSIGNALS)
			return -EIO;
		data = current_thread_info()->exec_domain->signal_map[data];
	}

	if (req == 1 || req == 2 || req == 3) {
		mm_segment_t	old_fs = get_fs();
		int		error;

		set_fs(get_ds());
		error = SYS(ptrace,req, pid, addr, (long)&res);
		set_fs(old_fs);

		if (error)
			return (error);
	}

#if defined(CONFIG_ABI_TRACE)
	if (req == 3 || req == 6) {
		abi_trace(ABI_TRACE_API, "%ld [%s] = 0x%08lx\n",
			addr >> 2, (addr >> 2) < ARRAY_SIZE(regnam) ?
				regnam[addr >> 2] : "???",
			req == 3 ? res : data);
	}
#endif

	if (req == 1 || req == 2 || req == 3)
		return (res);

	res = SYS(ptrace,req, pid, addr, data); return res;
}
