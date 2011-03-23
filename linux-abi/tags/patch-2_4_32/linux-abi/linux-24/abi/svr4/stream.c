/*
 * Copyright 1994,1995	Mike Jagdis (jaggy@purplet.demon.co.uk)
 */

#ident "%W% %G%"

#include <linux/config.h>
#include <linux/errno.h>
#include <linux/ptrace.h>
#include <linux/sched.h>
#include <linux/file.h>
#include <asm/uaccess.h>

#include <asm/abi_machdep.h>
#include <abi/tli.h>


#if !defined(CONFIG_ABI_XTI) && !defined(CONFIG_ABI_SPX)
#  define timod_getmsg(fd, ino, is_pmsg, regs)	0
#  define timod_putmsg(fd, ino, is_pmsg, regs)	0
#endif

/*
 * Check if the inode belongs to /dev/spx.
 */
#define IS_SPX(ip) ((MAJOR((ip)->i_rdev) == 30 && MINOR((ip)->i_rdev) == 1))


int
svr4_getmsg(struct pt_regs *regs)
{
	struct file		*fp;
	struct inode		*ip;
	int			fd;
	int			error = -EBADF;

	fd = (int)get_syscall_parameter(regs, 0);
	fp = fget(fd);
	if (fp) {
		ip = fp->f_dentry->d_inode;
		if (ip->i_sock)
			error = timod_getmsg(fd, ip, 0, regs);
		fput(fp);
	}
	return error;
}

int
svr4_putmsg(struct pt_regs *regs)
{
	struct file		*fp;
	struct inode		*ip;
	int			fd;
	int			error = -EBADF;

	fd = (int)get_syscall_parameter(regs, 0);
	fp = fget(fd);
	if (fp) {
		ip = fp->f_dentry->d_inode;
		if (ip->i_sock || IS_SPX(ip))
			error = timod_putmsg(fd, ip, 0, regs);
		fput(fp);
	}
	return error;
}

#ifdef CONFIG_ABI_XTI
int
svr4_getpmsg(struct pt_regs *regs)
{
	struct file		*fp;
	struct inode		*ip;
	int			fd;
	int			error = -EBADF;

	fd = (int)get_syscall_parameter(regs, 0);
	fp = fget(fd);
	if (fp) {
		ip = fp->f_dentry->d_inode;
		if (ip->i_sock)
			error = timod_getmsg(fd, ip, 1, regs);
		fput(fp);
	}
	return error;
}

int
svr4_putpmsg(struct pt_regs *regs)
{
	struct file		*fp;
	struct inode		*ip;
	int			fd;
	int			error = -EBADF;

	fd = (int)get_syscall_parameter(regs, 0);
	fp = fget(fd);
	if (fp) {
		ip = fp->f_dentry->d_inode;
		if (ip->i_sock || IS_SPX(ip))
			error = timod_putmsg(fd, ip, 1, regs);
		fput(fp);
	}
	return error;
}
#endif /* CONFIG_ABI_XTI */
