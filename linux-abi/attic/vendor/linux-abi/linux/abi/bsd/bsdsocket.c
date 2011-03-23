/*
 *  linux/ibcs/bsdsocket.c
 *
 *  Copyright (C) 1994  Mike Jagdis
 *
 * $Id$
 * $Source$
 */

#include <linux/config.h>

#include <linux/version.h>

#include <asm/uaccess.h>

#include <linux/mm.h>
#include <linux/net.h>
#include <linux/ptrace.h>
#include <linux/socket.h>
#include <linux/sys.h>

#include <abi/abi.h>
#include <abi/bsd.h>


int
bsd_connect(struct pt_regs *regs)
{
	int error;
	char *addr;
	unsigned short s;

	/* With BSD the first byte of the sockaddr struct is a length and
	 * the second is the address family. With Linux the address family
	 * occupies both bytes.
	 */
	addr = get_syscall_parameter (regs, 1);
	if ((error = verify_area(VERIFY_READ, addr, 2)))
		return error;
	get_user(s, (unsigned short *)addr);
	put_user(s>>8, (unsigned short *)addr);

	error = SYS(socketcall)(SYS_CONNECT, get_syscall_parameter (regs, 0));

	put_user(s, (unsigned short *)addr);
	return error;
}
