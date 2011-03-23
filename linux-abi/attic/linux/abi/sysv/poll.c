/*
 * This file contains the procedures for the handling of poll.
 *
 * Copyright (C) 1994 Eric Youngdale
 *
 * Created for Linux based loosely upon linux select code, which
 * in turn is loosely based upon Mathius Lattner's minix
 * patches by Peter MacDonald. Heavily edited by Linus.
 *
 * Poll is used by SVr4 instead of select, and it has considerably
 * more functionality.  Parts of it are related to STREAMS, and since
 * we do not have streams, we fake it.  In fact, select() still exists
 * under SVr4, but libc turns it into a poll() call instead.  We attempt
 * to do the inverse mapping.
 */

#include <linux/config.h>
#define __NO_VERSION__
#include <linux/module.h>
#include <linux/version.h>

#include <asm/uaccess.h>

#include <linux/types.h>
#include <linux/time.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/stat.h>
#include <linux/signal.h>
#include <linux/errno.h>
#include <linux/malloc.h>

#include <asm/system.h>

#include <abi/abi.h>
#include <abi/tli.h>
#include <abi/svr4.h>

#ifdef CONFIG_ABI_TRACE
#include <abi/trace.h>
#endif

/* FIXME: This is just copied from linux/fs/select.c simply so we can
 * add the XTI message check - and even then only if XTI is enabled
 * in the module. We could probably do better. But see the following
 * comment which notes the possibility of needing to do some flag
 * mapping in the event vectors one day...
 */

/* FIXME: Some of the event flags may need mapping. This list does
 * not agree with the list Linux is using. The important ones do, but...
 */
#define POLLIN 1
#define POLLPRI 2
#define POLLOUT 4
#define POLLERR 8
#define POLLHUP 16
#define POLLNVAL 32
#define POLLRDNORM 64
#define POLLWRNORM POLLOUT
#define POLLRDBAND 128
#define POLLWRBAND 256

#define DEFAULT_POLLMASK (POLLIN | POLLOUT | POLLRDNORM | POLLWRNORM)


int svr4_poll(struct poll * ufds, size_t nfds, int timeout)
{
	int error;

	/* FIXME: just have the callmap go direct to Linux poll()? */
#ifdef CONFIG_ABI_TRACE
	if ((ibcs_trace & TRACE_API) || ibcs_func_p->trace) {
		int i;
		for (i=0; i<nfds; i++) {
			printk(KERN_DEBUG "%d iBCS:      %3d 0x%04x 0x%04x\n",
				current->pid,
				ufds[i].fd, ufds[i].events, ufds[i].revents);
		}
	}
#endif

	error = SYS(poll)(ufds, nfds, timeout);

#ifdef CONFIG_ABI_TRACE
	if ((ibcs_trace & TRACE_API) || ibcs_func_p->trace) {
		int i;
		for (i=0; i<nfds; i++) {
			printk(KERN_DEBUG "%d iBCS:      %3d 0x%04x 0x%04x\n",
				current->pid,
				ufds[i].fd, ufds[i].events, ufds[i].revents);
		}
	}
#endif

	return error;
}

EXPORT_SYMBOL(svr4_poll);
