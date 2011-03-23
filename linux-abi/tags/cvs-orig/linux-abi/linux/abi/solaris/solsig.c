/*
 *  abi/solaris/solsig.c
 *
 *  Copyright (C) 1996  Miguel de Icaza (miguel@nuclecu.unam.mx)
 *
 */

#include <linux/config.h>

#include <linux/version.h>

#include <asm/uaccess.h>

#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/stddef.h>
#include <linux/unistd.h>
#include <linux/ptrace.h>
#include <linux/config.h>
#include <linux/fcntl.h>
#include <linux/personality.h>

#include <asm/system.h>
#include <linux/fs.h>
#include <linux/sys.h>

#include <abi/signal.h>

#include <abi/map.h>
#include <abi/bsd.h>
#include <abi/abi.h>

#include <abi/svr4sig.h>

/* My Solaris machine reports this
 * Maybe I should only provide those that Linux has?
 */
svr4_sigfillset (svr4_sigset_t *s)
{
	int i;
	
	s.setbits [0] = 0xffffffff; /* 32 signals */
	s.setbits [1] = 0; /* put 7ff here to be the same as Solaris 2.5 */
	s.setbits [2] = 0;
	s.setbits [3] = 0;
	return 0;
}
