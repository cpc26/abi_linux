/*
 *  linux/abi/secureware.c
 *
 *  Copyright (C) 1994  Mike Jagdis (jaggy@purplet.demon.co.uk)
 *
 * $Id$
 * $Source$
 *
 * SecureWare, Inc. provided the C2 security subsystem used on SCO Unix.
 * This is not that package. This does not even attempt to emulate
 * that package. This emulates just enough of the "obvious" bits to
 * allow some programs to get a bit further. It is not useful to
 * try to implement C2 security in an emulator. Nor is it particularly
 * useful to run SCO's secure admin programs on Linux anyway...
 */

#include <linux/config.h>

#include <linux/version.h>

#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/sched.h>

#include <abi/abi.h>

#ifdef CONFIG_ABI_TRACE
#include <abi/trace.h>
#endif


int
sw_security(int cmd, void *p1, void *p2, void *p3, void *p4, void *p5)
{
	switch (cmd) {
		case 1: /* getluid */
			/* We want the login user id. We don't have it
			 * specifically so we'll just use the real uid
			 * instead - it should be good enough.
			 */
			return current->uid;

		case 2: /* setluid */
			/* Strictly we should only be able to call setluid()
			 * once but we can't enforce that. We have the choice
			 * between having it always succeed or always fail.
			 * Since setluid() should only ever be invoked by
			 * things like login processes we always fail it.
			 */
			return -EPERM;

		case 0:
		case 3:
		case 4:
		default:
			printk(KERN_ERR "iBCS: unsupported security call cmd=%d\n",
				cmd);
			return -EINVAL;
	}
}
