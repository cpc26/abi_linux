/*
 *  linux/abi/svr4_common/ulimit.c
 *
 *  Copyright (C) 1993  Joe Portman (baron@hebron.connected.com)
 *	 First stab at ulimit
 *
 *  April 9 1994, corrected file size passed to/from setrlimit/getrlimit
 *    -- Graham Adams (gadams@ddrive.demon.co.uk)
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

#include <asm/system.h>
#include <linux/fs.h>
#include <linux/sys.h>
#include <linux/resource.h>
#include <linux/capability.h>

#include <abi/abi.h>
#include <abi/trace.h>
#include <abi/compat.h>

#define U_GETFSIZE 	(1)		  /* get max file size in blocks */
#define U_SETFSIZE 	(2)		  /* set max file size in blocks */
#define U_GETMEMLIM	(3)		  /* get process size limit */
#define U_GETMAXOPEN	(4)		  /* get max open files for this process */
#define U_GTXTOFF		(64)		  /* get text offset */
/*
 * Define nominal block size parameters.
 */
#define ULIM_BLOCKSIZE_BITS   9           /* block size = 512 */
#define ULIM_MAX_BLOCKSIZE (INT_MAX >> ULIM_BLOCKSIZE_BITS)

int svr4_ulimit (int cmd, int val)
{
	switch (cmd) {
		case U_GETFSIZE:
			return (current->rlim[RLIMIT_FSIZE].rlim_cur) >>
                                ULIM_BLOCKSIZE_BITS;

		case U_SETFSIZE:
			if ((val > ULIM_MAX_BLOCKSIZE) || (val < 0))
				return -ERANGE;
			val <<= ULIM_BLOCKSIZE_BITS;
			if (val > current->rlim[RLIMIT_FSIZE].rlim_max) {
				if (!capable(CAP_SYS_RESOURCE))
					return -EPERM;
				else {
					current->rlim[RLIMIT_FSIZE].rlim_max = val;
				}
			}
			current->rlim[RLIMIT_FSIZE].rlim_cur = val;
			return 0;

		case U_GETMEMLIM:
			return current->rlim[RLIMIT_DATA].rlim_cur;

		case U_GETMAXOPEN:
			return FDS_RLIMIT;

		default:
#ifdef CONFIG_ABI_TRACE
			if ((ibcs_trace & TRACE_API) || ibcs_func_p->trace) {
				printk(KERN_DEBUG "iBCS2: unsupported ulimit call %d\n", cmd);
			}
#endif
			return -EINVAL;
	 }
}

EXPORT_SYMBOL(svr4_ulimit);


#define U_RLIMIT_CPU	0
#define U_RLIMIT_FSIZE	1
#define U_RLIMIT_DATA	2
#define U_RLIMIT_STACK	3
#define U_RLIMIT_CORE	4
#define U_RLIMIT_NOFILE	5
#define U_RLIMIT_AS	6


int svr4_getrlimit(int cmd, void *val)
{
	switch (cmd) {
		case U_RLIMIT_CPU:
			cmd = RLIMIT_CPU; break;
		case U_RLIMIT_FSIZE:
			cmd = RLIMIT_FSIZE; break;
		case U_RLIMIT_DATA:
			cmd = RLIMIT_DATA; break;
		case U_RLIMIT_STACK:
			cmd = RLIMIT_STACK; break;
		case U_RLIMIT_CORE:
			cmd = RLIMIT_CORE; break;
		case U_RLIMIT_NOFILE:
			cmd = RLIMIT_NOFILE; break;
		case U_RLIMIT_AS:
			cmd = RLIMIT_AS; break;
		default:
			return -EINVAL;
	}

	return SYS(getrlimit)(cmd, val);
}

EXPORT_SYMBOL(svr4_getrlimit);

int svr4_setrlimit(int cmd, void *val)
{
	switch (cmd) {
		case U_RLIMIT_CPU:
			cmd = RLIMIT_CPU; break;
		case U_RLIMIT_FSIZE:
			cmd = RLIMIT_FSIZE; break;
		case U_RLIMIT_DATA:
			cmd = RLIMIT_DATA; break;
		case U_RLIMIT_STACK:
			cmd = RLIMIT_STACK; break;
		case U_RLIMIT_CORE:
			cmd = RLIMIT_CORE; break;
		case U_RLIMIT_NOFILE:
			cmd = RLIMIT_NOFILE; break;
		case U_RLIMIT_AS:
			cmd = RLIMIT_AS; break;
		default:
			return -EINVAL;
	}

	return SYS(getrlimit)(cmd, val);
}

EXPORT_SYMBOL(svr4_setrlimit);
