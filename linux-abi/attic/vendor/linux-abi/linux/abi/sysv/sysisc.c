/*
 *  abi/svr4_common/isc_sysisc.c
 *     --- enable ISC 4.0 executables (posix setostype)
 *
 *  some changes also done to callmap.inc: readlink, symlink, lstat
 *	 entry point numbers are differing from SYSVr4
 *
 *	1994-05-06 Karl Kiniger (ki@kretz.co.at)
 *
 */

#include <linux/config.h>
#define __NO_VERSION__
#include <linux/module.h>
#include <linux/version.h>

#include <linux/types.h>
#include <linux/errno.h>
#include <linux/sockios.h>
#include <linux/kernel.h>

#include <abi/abi.h>

#ifdef CONFIG_ABI_TRACE
#include <abi/trace.h>
#endif


int
isc_setostype(int arg1)
{
#ifdef CONFIG_ABI_TRACE
	if ((ibcs_trace & TRACE_API) || ibcs_func_p->trace) {
		printk(KERN_DEBUG "iBCS: sysisc: %s 0x%x\n",
			"ISC_SETOSTYPE", arg1);
	}
#endif
	return 0;
}
