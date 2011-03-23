/*
 *  linux/ibcs/stat.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *
 *  Hacked by Eric Youngdale for iBCS.
 *  Added to by Drew Sullivan.
 *
 * $Id$
 * $Source$
 */
#include <linux/config.h>
#define __NO_VERSION__
#include <linux/module.h>

#include <linux/version.h>

#include <linux/errno.h>
#include <linux/stat.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/file.h>
#include <linux/smp_lock.h>

#include <asm/uaccess.h>

#include <abi/abi.h>

#ifdef __sparc__
static int cp_abi_stat(struct inode * inode, struct ibcs_stat * statbuf)
{
	struct ibcs_stat tmp;

	memset ((void *) &tmp, 0, sizeof (tmp));
	tmp.st_dev = inode->i_dev;
	tmp.st_ino = inode->i_ino;
	tmp.st_mode = inode->i_mode;
	tmp.st_nlink = inode->i_nlink;
	tmp.st_uid = inode->i_uid;
	tmp.st_gid = inode->i_gid;
	tmp.st_rdev = inode->i_rdev;
	tmp.st_size = inode->i_size;
	tmp.st_atime.tv_sec = inode->i_atime;
	tmp.st_mtime.tv_sec = inode->i_mtime;
	tmp.st_ctime.tv_sec = inode->i_ctime;
	tmp.st_blksize = inode->i_blksize;
	tmp.st_blocks  = inode->i_blocks;
	return copy_to_user(statbuf,&tmp,sizeof(tmp)) ? -EFAULT : 0;
}

#else /* if not sparc... */

/*
 * Believe it or not, the original stat structure is compatible with ibcs2.
 * The xstat struct used by SVr4 is different than our new struct, but we will
 * deal with that later
 */
static int cp_abi_stat(struct inode * inode, struct ibcs_stat * statbuf)
{
	struct ibcs_stat tmp;

	/* Note that we have to fold a long inode number down to a short.
	 * This must match what happens in coff:ibcs_read() and
	 * open.c:svr4_getdents() since code that figures out cwd needs
	 * the inodes to match. Because it must match read() on a
	 * directory we have to avoid the situation where we end up
	 * with a zero inode value. A zero inode value in a read()
	 * on a directory indicates an empty directory slot.
	 */
	if ((unsigned long)inode->i_ino & 0xffff)
		tmp.st_ino = (unsigned long)inode->i_ino & 0xffff;
	else
		tmp.st_ino = 0xfffe;

	tmp.st_dev = inode->i_dev;
	tmp.st_mode = inode->i_mode;
	tmp.st_nlink = inode->i_nlink;
	tmp.st_uid = inode->i_uid;
	tmp.st_gid = inode->i_gid;
	tmp.st_rdev = inode->i_rdev;
	tmp.st_size = inode->i_size;
	tmp.st_atime = inode->i_atime;
	tmp.st_mtime = inode->i_mtime;
	tmp.st_ctime = inode->i_ctime;
	return copy_to_user(statbuf,&tmp,sizeof(tmp)) ? -EFAULT : 0;
}
#endif /* not sparc */

int abi_stat(char * filename, struct ibcs_stat * statbuf)
{
	struct dentry * dentry;
	int error;

	lock_kernel();
	dentry = namei(filename);

	error = PTR_ERR(dentry);
	if (!IS_ERR(dentry)) {
		error = do_revalidate(dentry);
		if (!error)
			error = cp_abi_stat(dentry->d_inode, statbuf);

		dput(dentry);
	}
	unlock_kernel();
	return error;
}

EXPORT_SYMBOL(abi_stat);

int abi_lstat(char * filename, struct ibcs_stat * statbuf)
{
	struct dentry * dentry;
	int error;

	lock_kernel();
	dentry = lnamei(filename);

	error = PTR_ERR(dentry);
	if (!IS_ERR(dentry)) {
		error = do_revalidate(dentry);
		if (!error)
			error = cp_abi_stat(dentry->d_inode, statbuf);

		dput(dentry);
	}
	unlock_kernel();
	return error;
}

EXPORT_SYMBOL(abi_lstat);

int abi_fstat(unsigned int fd, struct ibcs_stat * statbuf)
{
	struct file * f;
	int err = -EBADF;

	lock_kernel();
	f = fget(fd);
	if (f) {
		struct dentry * dentry = f->f_dentry;

		err = do_revalidate(dentry);
		if (!err)
			err = cp_abi_stat(dentry->d_inode, statbuf);
		fput(f);
	}
	unlock_kernel();
	return err;
}

EXPORT_SYMBOL(abi_fstat);
