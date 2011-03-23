/*
 * Copyright (c) 2001 Christoph Hellwig.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

//#ident "%W% %G%"

/*
 * Misc SCO syscalls.
 */
#include "../include/util/i386_std.h"
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/file.h>
#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/stat.h>
#include <linux/syscalls.h>
#include <linux/fs.h>
#include <asm/uaccess.h>

#include "../include/svr4/sysent.h"
#include "../include/util/trace.h"
#if _KSL < 14
#define files_fdtable(x) x
#endif
#if _KSL > 25
#include <linux/fdtable.h>
#endif


int
sco_lseek(int fd, u_long offset, int whence)
{
	int		error;
	struct file	*fp;
	struct inode	*ip;

	if (offset == 0x40000000) {	/* Rewind Directory */
		fp = fget(fd);
		if (fp) {
			ip = fp->f_dentry->d_inode;
			if (ip && S_ISDIR(ip->i_mode)) offset = 0;
			fput(fp);
		}
	}
	error = SYS(lseek,fd, offset, whence);
	if (error == -ESPIPE) {
		fp = fget(fd);
		if (fp == NULL)
			goto out;
		ip = fp->f_dentry->d_inode;
		if (ip && (S_ISCHR(ip->i_mode) || S_ISBLK(ip->i_mode)))
			error = 0;
		fput(fp);
	}
out:
	return (error);
}

int
sco_fcntl(int fd, u_int cmd, u_long arg)
{
	/*
	 * This could be SCO's get highest fd open if the fd we
	 * are called on is -1 otherwise it could be F_CHKFL.
	 */
	if (fd == -1 && cmd == 8) {
		struct files_struct *files = current->files;
		int rval;

                /* compare to ./fs/open.c: get_unused_fd */
		spin_lock(&files->file_lock);
#if _KSL > 19
		rval = find_first_zero_bit(files_fdtable(files)->open_fds->fds_bits, files_fdtable(files)->max_fds);
#else
		rval = find_first_zero_bit(files_fdtable(files)->open_fds->fds_bits, files_fdtable(files)->max_fdset);
#endif
		spin_unlock(&files->file_lock);

		return rval;
	}

	return svr4_fcntl(fd, cmd, arg);
}

int
sco_sysi86(int cmd, void *arg1, int arg2)
{
	switch (cmd) {
	case 114 /*SI86GETFEATURES*/ :
		/*
		 * No, I don't know what these feature flags actually
		 * _mean_. This vector just matches SCO OS 5.0.0.
		 */
#define OSR5_FEATURES	"\001\001\001\001\001\001\001\001\002\001\001\001"
		arg2 = max(arg2, 12);
		if (copy_to_user(arg1, OSR5_FEATURES, arg2))
			return -EFAULT;
		return arg2;
	default:
		return svr4_sysi86(cmd, arg1, arg2);
	}
}

struct scodir {
	unsigned int inode;
	unsigned int offset;
	short len;
};
int sco_getdents(int fd, char *dirent, int count)
{
	int rval;
	unsigned int Pos, Node;
	char *p;
	short j;
	struct scodir *pEnt;

	rval = SYS(getdents,fd,dirent,count);
	if (rval <= 0) return rval;

	Pos = 0; p = dirent;	
	while (Pos < rval) {
		pEnt = (struct scodir *)p;
		get_user(j,&pEnt->len);
		Pos += j;
		put_user(Pos,&pEnt->offset);
		p += j;
		get_user(Node,&pEnt->inode);
#ifndef CONFIG_ABI_SHINOMAP
		Node = abi_map(Node,1);
#else
		Node = linux_to_svr4_ino_t(abi_map(Node,1));
#endif
		put_user(Node,&pEnt->inode);
	}
	return rval;
}
