/*
 *  linux/abi/bsd/bsdstat.c
 *
 *  Copyright (C) 1994  Mike Jagdis
 *
 * $Id$
 * $Source$
 */

#include <linux/config.h>

#include <linux/version.h>

#include <asm/uaccess.h>

#include <linux/errno.h>
#include <linux/stat.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/file.h>
#include <linux/smp_lock.h>

#include <abi/bsd.h>
#include <abi/abi.h>


static int
cp_bsd_stat(struct inode *inode, struct bsd_stat *st)
{
	struct bsd_stat tmp;

	tmp.st_dev = inode->i_dev;
	tmp.st_ino = inode->i_ino;
	tmp.st_mode = inode->i_mode;
	tmp.st_nlink = inode->i_nlink;
	tmp.st_uid = inode->i_uid;
	tmp.st_gid = inode->i_gid;
	tmp.st_rdev = inode->i_rdev;
	tmp.st_size = inode->i_size;
	tmp.st_atime = inode->i_atime;
	tmp.st_mtime = inode->i_mtime;
	tmp.st_ctime = inode->i_ctime;
	tmp.st_blksize = inode->i_blksize;
	tmp.st_blocks = inode->i_blocks;
	tmp.st_flags = inode->i_flags;
	tmp.st_gen = 0;

	return copy_to_user(st, &tmp, sizeof(struct bsd_stat));
}


int
bsd_stat(char *filename, struct bsd_stat *st)
{
	struct dentry * dentry;
	int error;

	lock_kernel();
	dentry = namei(filename);

	error = PTR_ERR(dentry);
	if (!IS_ERR(dentry)) {
		error = do_revalidate(dentry);
		if (!error)
			error = cp_bsd_stat(dentry->d_inode, st);

		dput(dentry);
	}
	unlock_kernel();
	return error;
}

int
bsd_lstat(char *filename, struct bsd_stat *st)
{
	struct dentry * dentry;
	int error;

	lock_kernel();
	dentry = lnamei(filename);

	error = PTR_ERR(dentry);
	if (!IS_ERR(dentry)) {
		error = do_revalidate(dentry);
		if (!error)
			error = cp_bsd_stat(dentry->d_inode, st);

		dput(dentry);
	}
	unlock_kernel();
	return error;
}

int
bsd_fstat(unsigned int fd, struct bsd_stat *st)
{
	struct file * file;
	int error = -EBADF;

	lock_kernel();
	file = fget(fd);
	if (file) {
		struct dentry * dentry = file->f_dentry;

		error = do_revalidate(dentry);
		if (!error)
			error = cp_bsd_stat(dentry->d_inode, st);
		fput(file);
	}
	unlock_kernel();
	return error;
}
