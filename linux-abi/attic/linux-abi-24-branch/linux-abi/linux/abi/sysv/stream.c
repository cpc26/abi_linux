/*
 *  abi/svr4_common/stream.c
 *
 *  Copyright 1994, 1995  Mike Jagdis (jaggy@purplet.demon.co.uk)
 *
 * $Id$
 * $Source$
 */

#include <linux/config.h>
#define __NO_VERSION__
#include <linux/module.h>
#include <linux/version.h>

#include <asm/uaccess.h>

#include <linux/types.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/ptrace.h>
#include <linux/net.h>
#include <linux/mm.h>
#include <linux/socket.h>
#include <linux/file.h>

#include <abi/abi.h>
#include <abi/stream.h>
#include <abi/tli.h>

#ifdef CONFIG_ABI_TRACE
#include <abi/trace.h>
#endif


int svr4_getmsg(struct pt_regs *regs)
{
	int fd;
	struct file *filep;
	struct inode *ino;
	int error;

	fd = (int)get_syscall_parameter (regs, 0);
	filep = fget(fd);
	if (!filep)
		return -EBADF;

	error = -EBADF;
	ino = filep->f_dentry->d_inode;
	if (ino->i_sock) {
#if defined(CONFIG_ABI_XTI) || defined(CONFIG_ABI_SPX)
		error = timod_getmsg(fd, ino, 0, regs);
#else
		error = 0;
#endif /* CONFIG_ABI_XTI */
	}
	fput(filep);
	return error;
}

EXPORT_SYMBOL(svr4_getmsg);


int svr4_putmsg(struct pt_regs *regs)
{
	int fd;
	struct file *filep;
	struct inode *ino;
	int error;

	fd = (int)get_syscall_parameter (regs, 0);
	filep = fget(fd);
	if (!filep)
		return -EBADF;

	error = -EBADF;
	ino = filep->f_dentry->d_inode;
	if (ino->i_sock
	|| (MAJOR(ino->i_rdev) == 30 && MINOR(ino->i_rdev) == 1)) {
#if defined(CONFIG_ABI_XTI) || defined(CONFIG_ABI_SPX)
		error = timod_putmsg(fd, ino, 0, regs);
#else
		error = 0;
#endif
	}
	fput(filep);
	return error;
}

EXPORT_SYMBOL(svr4_putmsg);

#ifdef CONFIG_ABI_XTI
int svr4_getpmsg(struct pt_regs *regs)
{
	int fd;
	struct file *filep;
	struct inode *ino;
	int error;

	fd = (int)get_syscall_parameter (regs, 0);

	filep = fget(fd);
	if (!filep)
		return -EBADF;

	error = -EBADF;
	ino = filep->f_dentry->d_inode;
	if (ino->i_sock)
		error = timod_getmsg(fd, ino, 1, regs);
	fput(filep);
	return error;
}

EXPORT_SYMBOL(svr4_getpmsg);

int svr4_putpmsg(struct pt_regs *regs)
{
	int fd;
	struct file *filep;
	struct inode *ino;
	int error;

	fd = (int)get_syscall_parameter (regs, 0);

	filep = fget(fd);
	if (!filep)
		return -EBADF;

	error = -EBADF;
	ino = filep->f_dentry->d_inode;
	if (ino->i_sock
	|| (MAJOR(ino->i_rdev) == 30 && MINOR(ino->i_rdev) == 1))
		error = timod_putmsg(fd, ino, 1, regs);
	fput(filep);
	return error;
}

EXPORT_SYMBOL(svr4_putpmsg);

#endif /* CONFIG_ABI_XTI */
