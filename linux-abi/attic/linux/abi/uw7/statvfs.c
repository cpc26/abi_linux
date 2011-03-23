/*
 *    abi/uw7/statvfs.c - statvfs64() and friends for UW7
 *
 *  This software is under GPL
 */

#include <linux/sched.h>
#include <linux/file.h>
#include <linux/smp_lock.h>

#include <asm/uaccess.h>

#include <abi/uw7.h>

#undef DEBUG

#ifdef DEBUG
#define DBG(x...)	printk(x)
#else
#define DBG(x...)
#endif

static int cp_uw7_statvfs64(struct super_block * sb, struct statfs * src, 
	struct uw7_statvfs64 * dst)
{
	struct uw7_statvfs64 tmp = {0, };

	tmp.f_bsize = src->f_bsize;
	tmp.f_frsize = src->f_bsize;
	tmp.f_blocks = src->f_blocks;;
	tmp.f_bfree = src->f_bfree;
	tmp.f_bavail = src->f_bavail;
	tmp.f_files = src->f_files;
	tmp.f_ffree = src->f_ffree;
	tmp.f_favail = src->f_ffree;
	tmp.f_fsid = sb->s_dev;
	strcpy(tmp.f_basetype, sb->s_type->name);
	tmp.f_flag = 0;
	tmp.f_namemax = src->f_namelen;
	return copy_to_user(dst, &tmp, sizeof(tmp)) ? -EFAULT : 0;
}

int uw7_statvfs64(char * filename, struct uw7_statvfs64 * buf)
{
	struct statfs st;
	int error;
	mm_segment_t old_fs;
	struct super_block * sb;
	struct dentry * dentry;

	dentry = namei(filename);
	error = PTR_ERR(dentry);
	if (!IS_ERR(dentry)) {
		sb = dentry->d_inode->i_sb;

		old_fs = get_fs();
		set_fs(get_ds());
		lock_kernel();
		if (sb && sb->s_op && sb->s_op->statfs)
			error = sb->s_op->statfs(sb, &st);
		else
			error = -ENODEV;
		unlock_kernel();
		set_fs(old_fs);
		dput(dentry);
	}
	return error ? : cp_uw7_statvfs64(sb, &st, buf);
}

int uw7_fstatvfs64(int fd, struct uw7_statvfs64 * buf)
{
	struct statfs st;
	int error = -EBADF;
	mm_segment_t old_fs;
	struct super_block * sb;
	struct file * filep;

	filep = fget(fd);
	if (!filep) 
		goto out;
	sb = filep->f_dentry->d_inode->i_sb;
	old_fs = get_fs();
	set_fs(get_ds());
	lock_kernel();
	if (sb && sb->s_op && sb->s_op->statfs)
		error = sb->s_op->statfs(sb, &st);
	else
		error = -ENODEV;
	unlock_kernel();
	set_fs(old_fs);
	fput(filep);
out:
	return error ? : cp_uw7_statvfs64(sb, &st, buf);
}
