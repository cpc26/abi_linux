/*
 *  linux/abi/svr4_common/sysconf.c
 *
 *  Copyright (C) 1994  Mike Jagdis (jaggy@purplet.demon.co.uk)
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
#include <linux/limits.h>
#include <linux/unistd.h>
#include <linux/ptrace.h>
#include <linux/fcntl.h>
#include <linux/smp.h>
#include <linux/swap.h>

#include <asm/system.h>
#include <linux/fs.h>
#include <linux/sys.h>

#include <abi/abi.h>
#include <abi/svr4.h>

#ifdef CONFIG_ABI_TRACE
#include <abi/trace.h>
#endif


/* The sysconf() call is supposed to give applications access to various
 * kernel parameters. According to SCO's man page this a POSIX mandated
 * function. Perhaps it should be moved across as a native Linux call?
 *
 * N.B. SCO only has sysconf in the Xenix group. Therefore this is based
 * on the Xenix spec. Is SVR4 the same? Wyse Unix V.3.2.1A doesn't have
 * sysconf documented at all.
 *
 * N.B. 0-7 are required (by who?). Other values may be defined for
 * various systems but there appears no guarantee that they match across
 * platforms. Thus, unless we can identify what system the executable
 * was compiled for, we probably prefer to have extensions fail. Hell,
 * nothing important is going to use this obscure stuff anyway...
 */
#define _SC_ARG_MAX	0
#define _SC_CHILD_MAX	1
#define _SC_CLK_TCK	2
#define _SC_NGROUPS_MAX	3
#define _SC_OPEN_MAX	4
#define _SC_JOB_CONTROL	5
#define _SC_SAVED_IDS	6
#define _SC_VERSION	7

#define _SC_PAGESIZE		11
#define _SCO_SC_PAGESIZE	34


/* This is an SVR4 system call that is undocumented except for some
 * hints in a header file. It appears to be a forerunner to the
 * POSIX sysconf() call.
 */
int svr4_sysconfig(int name)
{
	switch (name) {
		case _CONFIG_NGROUPS:
			/* From limits.h */
			return (NGROUPS_MAX);

		case _CONFIG_CHILD_MAX:
			/* From limits.h */
			return (CHILD_MAX);

		case _CONFIG_OPEN_FILES:
			/* From limits.h */
			return (OPEN_MAX);

		case _CONFIG_POSIX_VER:
			/* The version of the POSIX standard we conform
			 * to. SCO defines _POSIX_VERSION as 198808L
			 * sys/unistd.h. What are we? We are 199009L.
			 */
			return (199009L);

		case _CONFIG_PAGESIZE:
			return (PAGE_SIZE);

		case _CONFIG_CLK_TCK:
			return (HZ);

		case _CONFIG_XOPEN_VER:
			return 4;

		case _CONFIG_NACLS_MAX:
			return 0;

		case _CONFIG_NPROC:
			return 4000;
		
		case _CONFIG_NENGINE:
		case _CONFIG_NENGINE_ONLN:
			return (smp_num_cpus);

		case _CONFIG_TOTAL_MEMORY:
			return (max_mapnr << (PAGE_SHIFT-10));	

		case _CONFIG_USEABLE_MEMORY:
		case _CONFIG_GENERAL_MEMORY:
			return ((unsigned long) (nr_free_pages()) << (PAGE_SHIFT-10));

		case _CONFIG_DEDICATED_MEMORY:
			return 0;

		case _CONFIG_NCGS_CONF:
		case _CONFIG_NCGS_ONLN:
		case _CONFIG_MAX_ENG_PER_CG:
			return 1; /* no NUMA-Q support on Linux yet */

		case _CONFIG_CACHE_LINE:
			return 32; /* XXX is there a more accurate way? */

		case _CONFIG_KERNEL_VM:
			return -EINVAL;

		case _CONFIG_ARG_MAX:
			/* From limits.h */
			return (ARG_MAX);
	}
#ifdef CONFIG_ABI_TRACE
	if ((ibcs_trace & TRACE_API) || ibcs_func_p->trace) {
		printk(KERN_DEBUG "iBCS2 unsupported sysconfig call %d\n", name);
	}
#endif

	return -EINVAL;
}


int ibcs_sysconf(int name)
{
	switch (name) {
		case _SC_ARG_MAX:
			/* From limits.h */
			return (ARG_MAX);

		case _SC_CHILD_MAX:
			/* From limits.h */
			return (CHILD_MAX);

		case _SC_CLK_TCK:
			return (HZ);

		case _SC_NGROUPS_MAX:
			/* From limits.h */
			return (NGROUPS_MAX);

		case _SC_OPEN_MAX:
			/* From limits.h */
			return (OPEN_MAX);

		case _SC_JOB_CONTROL:
			return (1);

		case _SC_SAVED_IDS:
			return (1);

		case _SC_PAGESIZE:
		case _SCO_SC_PAGESIZE:
			return PAGE_SIZE;

		case _SC_VERSION:
			/* The version of the POSIX standard we conform
			 * to. SCO defines _POSIX_VERSION as 198808L
			 * sys/unistd.h. What are we?
			 */
			return (198808L);
	}

#ifdef CONFIG_ABI_TRACE
	if ((ibcs_trace & TRACE_API) || ibcs_func_p->trace) {
		printk(KERN_DEBUG "iBCS2 unsupported sysconf call %d\n", name);
	}
#endif

	return -EINVAL;
}

EXPORT_SYMBOL(svr4_sysconfig);
