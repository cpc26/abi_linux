/*
 *    abi/svr4_common/xstat.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *
 *  Hacked by Eric Youngdale for iBCS (1993, 1994).
 *  Added to by Drew Sullivan, modified by EY for xstat (used by SVr4).
 *
 * $Id$
 * $Source$
 */

#include <linux/config.h>
#define __NO_VERSION__
#include <linux/module.h>
#include <linux/version.h>

#include <asm/uaccess.h>

#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/sys.h>
#include <linux/file.h>
#include <linux/smp_lock.h>

#include <abi/abi.h>
#include <abi/abi4.h>
#include <abi/trace.h>
#include <abi/lfs.h>

/*
 * The xstat interface is used by SVr4, and is effectively an extension
 * to stat.  The general idea is that stat has some limitations (chiefly
 * 16 bit inode numbers), and the solution in SVr4 was to add an entirely
 * new syscall.  The /usr/include/sys/stat.h header file defines stat as xstat
 * so that the new interface is used.  The one advantage of xstat is that
 * we pass a version number so that it is possible to tell exactly what
 * the application is expecting, and it is easy to do the right thing.
 * There is usually an inline wrapper function in /usr/include/sys/stat.h
 * to perform this conversion.
 */

#define R3_MKNOD_VERSION	1	/* SVr3 */
#define R4_MKNOD_VERSION	2	/* SVr4 */
#define R3_STAT_VERSION		1	/* SVr3 */
#define R4_STAT_VERSION		2	/* SVr4 */
#define UW7_XSTAT64_VERSION	4	/* UW7 xstat64 */
#define SCO_STAT_VERSION	51	/* SCO OS5 */

/* Various functions to provide compatibility between the linux
   syscalls and the ABI ABI compliant calls */

/* Convert a linux dev number into the SVr4 equivalent. */
#define R4_DEV(DEV) ((DEV & 0xff) | ((DEV & 0xff00) << 10))


struct sco_xstat {
	short		st_dev;
	long		__pad1[3];
	unsigned long	st_ino;
	unsigned short	st_mode;
	short		st_nlink;
	unsigned short	st_uid;
	unsigned short	st_gid;
	short		st_rdev;
	long		__pad2[2];
	long		st_size;
	long		__pad3;
	long		st_atime;
	long		st_mtime;
	long		st_ctime;
	long		st_blksize;
	long		st_blocks;
	char		st_fstype[16];
	long		__pad4[7];
	long		st_sco_flags;
};



/*
 * st_blocks and st_blksize are approximated with a simple algorithm if
 * they aren't supported directly by the filesystem. The minix and msdos
 * filesystems don't keep track of blocks, so they would either have to
 * be counted explicitly (by delving into the file itself), or by using
 * this simple algorithm to get a reasonable (although not 100% accurate)
 * value.
 *
 * Use minix fs values for the number of direct and indirect blocks.  The
 * count is now exact for the minix fs except that it counts zero blocks.
 * Everything is in BLOCK_SIZE'd units until the assignment to
 * tmp.st_blksize.
 */
static void
set_blocks(struct inode *inode, long *st_blksize, long *st_blocks)
{
	long blocks, indirect;

#define D_B   7
#define I_B   (BLOCK_SIZE / sizeof(unsigned short))

	if (!inode->i_blksize) {
		blocks = (inode->i_size + BLOCK_SIZE - 1) / BLOCK_SIZE;
		if (blocks > D_B) {
			indirect = (blocks - D_B + I_B - 1) / I_B;
			blocks += indirect;
			if (indirect > 1) {
				indirect = (indirect - 1 + I_B - 1) / I_B;
				blocks += indirect;
				if (indirect > 1)
					blocks++;
			}
		}
		*st_blocks = (BLOCK_SIZE / 512) * blocks;
		*st_blksize = BLOCK_SIZE;
	} else {
		*st_blocks = inode->i_blocks;
		*st_blksize = inode->i_blksize;
	}
}


static int
cp_sco_xstat(struct inode * inode, struct sco_xstat * statbuf)
{
	struct sco_xstat tmp = {0, };

	/* If we thought we were in a short inode environment we are
	 * probably already too late - getdents() will have likely
	 * already assumed short inodes and "fixed" anything with
	 * a zero low word (because it must match stat() which must
	 * match read() on a directory). We will just have to go
	 * along with it.
	 */
	if ((current->personality & SHORT_INODE)
	&& !((unsigned long)tmp.st_ino & 0xffff))
		tmp.st_ino = 0xfffffffe;
	else
		tmp.st_ino = inode->i_ino;
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
	set_blocks(inode, &tmp.st_blksize, &tmp.st_blocks);
	strcpy(tmp.st_fstype, "ext2");
	tmp.st_sco_flags = 0; /* 1 if remote */
	return copy_to_user(statbuf, &tmp, sizeof(tmp)) ? -EFAULT : 0;
}


static int
cp_svr4_xstat(struct inode * inode, struct svr4_xstat * statbuf)
{
	struct svr4_xstat tmp = {0, };

	tmp.st_dev = R4_DEV(inode->i_dev);
	tmp.st_ino = inode->i_ino;
	tmp.st_mode = inode->i_mode;
	tmp.st_nlink = inode->i_nlink;
	tmp.st_uid = inode->i_uid;
	tmp.st_gid = inode->i_gid;
	tmp.st_rdev = R4_DEV(inode->i_rdev);
	tmp.st_size = inode->i_size;
	tmp.st_atim.tv_sec = inode->i_atime;
	tmp.st_mtim.tv_sec = inode->i_mtime;
	tmp.st_ctim.tv_sec = inode->i_ctime;
	set_blocks(inode, &tmp.st_blksize, &tmp.st_blocks);
	strcpy(tmp.st_fstype, "ext2");
	return copy_to_user(statbuf, &tmp, sizeof(tmp)) ? -EFAULT : 0;
}

static int
cp_uw7_stat64(struct inode * inode, struct uw7_stat64 * statbuf)
{
	struct uw7_stat64 tmp = {0, };

	tmp.st_dev = inode->i_dev;
	tmp.st_ino = (unsigned long long)inode->i_ino;
	tmp.st_mode = (unsigned long)inode->i_mode;
	tmp.st_nlink = inode->i_nlink;
	tmp.st_uid = inode->i_uid;
	tmp.st_gid = inode->i_gid;
	tmp.st_rdev = inode->i_rdev; /* XXX check this! */
	tmp.st_size = (long long)inode->i_size;
	tmp.st_atime.tv_sec = inode->i_atime;
	tmp.st_atime.tv_usec = 0L;
	tmp.st_mtime.tv_sec = inode->i_mtime;
	tmp.st_mtime.tv_usec = 0L;
	tmp.st_ctime.tv_sec = inode->i_ctime;
	tmp.st_ctime.tv_usec = 0L;
	set_blocks(inode, &tmp.st_blksize, (long *)&tmp.st_blocks);
	strcpy(tmp.st_fstype, "ext2");
	tmp.st_aclcnt = 0;
	tmp.st_level = 0;
	tmp.st_flags = 0;
	tmp.st_cmwlevel = 0;
	
	return copy_to_user(statbuf, &tmp, sizeof(tmp)) ? -EFAULT : 0;
}

int svr4_xstat(int vers, char * filename, void * buf)
{
	struct dentry * dentry;
	int error;

	if (vers == R3_STAT_VERSION)
		return abi_stat(filename, (struct ibcs_stat *)buf);

	lock_kernel();
	dentry = namei(filename);

	error = PTR_ERR(dentry);
	if (!IS_ERR(dentry)) {
		error = do_revalidate(dentry);
		if (!error) switch (vers) {
			case R4_STAT_VERSION:
				error = cp_svr4_xstat(dentry->d_inode, buf);
				break;

			case SCO_STAT_VERSION:
				error = cp_sco_xstat(dentry->d_inode, buf);
				break;

			case UW7_XSTAT64_VERSION:
				error = cp_uw7_stat64(dentry->d_inode, buf);
				break;

			default:
				error = -EINVAL;
#ifdef CONFIG_ABI_TRACE
				if (ibcs_trace & TRACE_API)
					printk(KERN_DEBUG
						"iBCS: [%d] xstat version %d not supported\n",
						current->pid, vers);
#endif
				break;
		}

		dput(dentry);
	}
	unlock_kernel();
	return error;
}

EXPORT_SYMBOL(svr4_xstat);


int svr4_lxstat(int vers, char * filename, void * buf)
{
	struct dentry * dentry;
	int error;

	if (vers == R3_STAT_VERSION)
		return abi_lstat(filename, (struct ibcs_stat *)buf);

	lock_kernel();
	dentry = lnamei(filename);

	error = PTR_ERR(dentry);
	if (!IS_ERR(dentry)) {
		error = do_revalidate(dentry);
		if (!error) switch(vers) {
			case R4_STAT_VERSION:
				error = cp_svr4_xstat(dentry->d_inode, buf);
				break;

			case SCO_STAT_VERSION:
				error = cp_sco_xstat(dentry->d_inode, buf);
				break;

			case UW7_XSTAT64_VERSION:
				error = cp_uw7_stat64(dentry->d_inode, buf);
				break;

			default:
#ifdef CONFIG_ABI_TRACE
				if (ibcs_trace & TRACE_API)
					printk(KERN_DEBUG
						"iBCS: [%d] lxstat version %d not supported\n",
						current->pid, vers);
#endif
				error = -EINVAL;
		}

		dput(dentry);
	}
	unlock_kernel();
	return error;
}

EXPORT_SYMBOL(svr4_lxstat);

int svr4_fxstat(int vers, int fd, void *buf)
{
	struct file * f;
	int error = -EBADF;

	if (vers == R3_STAT_VERSION)
		return abi_fstat(fd, (struct ibcs_stat *)buf);

	lock_kernel();
	f = fget(fd);
	if (f) {
		struct dentry * dentry = f->f_dentry;

		error = do_revalidate(dentry);
		if (!error) switch(vers) {
			case R4_STAT_VERSION:
				error = cp_svr4_xstat(dentry->d_inode, buf);
				break;

			case SCO_STAT_VERSION:
				error = cp_sco_xstat(dentry->d_inode, buf);
				break;

			case UW7_XSTAT64_VERSION:
				error = cp_uw7_stat64(dentry->d_inode, buf);
				break;

			default:
#ifdef CONFIG_ABI_TRACE
				if (ibcs_trace & TRACE_API)
					printk(KERN_DEBUG
						"iBCS: [%d] fxstat version %d not supported\n",
						current->pid, vers);
#endif
				error = -EINVAL;
		}

		fput(f);
	}
	unlock_kernel();
	return error;
}

EXPORT_SYMBOL(svr4_fxstat);


int svr4_xmknod(int vers, const char * path, mode_t mode, dev_t dev)
{
	unsigned int major, minor;

	switch(vers) {
		case R3_MKNOD_VERSION:
			return abi_mknod(path, mode, dev);

		case R4_MKNOD_VERSION:
			minor = dev & 0x3ffff;
			major = dev >> 18;
			if (minor > 0xff || major > 0xff)
				return -EINVAL;
			return abi_mknod(path, mode, ((major << 8) | minor));
	}

#ifdef CONFIG_ABI_TRACE
	if (ibcs_trace & TRACE_API)
		printk(KERN_DEBUG "iBCS: [%d] xmknod version %d not supported\n",
			current->pid, vers);
#endif
	return -EINVAL;
}

EXPORT_SYMBOL(svr4_xmknod);


/*
 * The following code implements the statvfs function used by SVr4.
 */

static int
cp_abi_statvfs(struct dentry *dentry,
		struct statfs *src, struct abi_statvfs *statbuf)
{
	struct inode *inode = dentry->d_inode;
	struct abi_statvfs tmp = {0, };

	tmp.f_blocks = src->f_blocks;

	tmp.f_bsize = src->f_bsize; 
	tmp.f_frsize = 0;
	tmp.f_blocks = src->f_blocks;;
	tmp.f_bfree = src->f_bfree;
	tmp.f_bavail = src->f_bavail;
	tmp.f_files = src->f_files;
	tmp.f_free = src->f_ffree;
	tmp.f_sid = inode->i_sb->s_dev;

	/* Get the name of the filesystem */
	strcpy(tmp.f_basetype, inode->i_sb->s_type->name);

	tmp.f_flag = 0;
	if (IS_RDONLY(inode)) tmp.f_flag |= 1;
	if (IS_NOSUID(inode)) tmp.f_flag |= 2;

	tmp.f_namemax = src->f_namelen;
    
	return copy_to_user(statbuf, &tmp, sizeof(tmp)) ? -EFAULT : 0;
}


static int
cp_sco_statvfs(struct dentry *dentry,
		struct statfs *src, struct sco_statvfs *statbuf)
{
	struct inode *inode = dentry->d_inode;
	struct sco_statvfs tmp = {0, };

	tmp.f_blocks = src->f_blocks;

	tmp.f_bsize = src->f_bsize; 
	tmp.f_frsize = src->f_bsize;
	tmp.f_blocks = src->f_blocks;;
	tmp.f_bfree = src->f_bfree;
	tmp.f_bavail = src->f_bavail;
	tmp.f_files = src->f_files;
	tmp.f_free = src->f_ffree;
	tmp.f_favail = src->f_ffree; /* SCO addition in the middle! */
	tmp.f_sid = inode->i_sb->s_dev;

	/* Get the name of the filesystem. Sadly, some code
	 * "in the wild" actually checks the name against a
	 * hard coded list to see if it is a "real" fs or not.
	 * I believe Informix Dynamic Server for SCO is one such.
	 * More lies...
	 */
	if (personality(PER_SCOSVR3)
	&& !strncmp(inode->i_sb->s_type->name, "ext2", 4)) {
		strcpy(tmp.f_basetype, "HTFS");
	} else {
		strcpy(tmp.f_basetype, inode->i_sb->s_type->name);
	}

	tmp.f_flag = 0;
	if (IS_RDONLY(inode)) tmp.f_flag |= 1;
	if (IS_NOSUID(inode)) tmp.f_flag |= 2;

	tmp.f_namemax = src->f_namelen;

	return copy_to_user(statbuf, &tmp, sizeof(tmp)) ? -EFAULT : 0;
}


int abi_statvfs(char * filename, void * stat)
{
	int error;
	struct statfs st;
	mm_segment_t old_fs;
	struct dentry *dentry;

	lock_kernel();
	dentry = namei(filename);

	error = PTR_ERR(dentry);
	if (!IS_ERR(dentry)) {
		error = do_revalidate(dentry);

		old_fs = get_fs();
		set_fs(get_ds());
		error = SYS(statfs)(filename, &st);
		set_fs(old_fs);

		if (!error) {
			if (personality(PER_SCOSVR3))
				error = cp_sco_statvfs(dentry, &st, stat);
			else
				error = cp_abi_statvfs(dentry, &st, stat);
		}

		dput(dentry);
	}
	unlock_kernel();
	return error;
}

EXPORT_SYMBOL(abi_statvfs);

int abi_fstatvfs(int fd, void *stat)
{
	int error = -EBADF;
	struct statfs st;
	mm_segment_t old_fs;
	struct file *filep;

	lock_kernel();
	filep = fget(fd);
	if (filep) {
		old_fs = get_fs();
		set_fs(get_ds());
		error = SYS(fstatfs)(fd, &st);
		set_fs(old_fs);

		if (!error) {
			if (personality(PER_SCOSVR3))
				error = cp_sco_statvfs(filep->f_dentry, &st, stat);
			else
				error = cp_abi_statvfs(filep->f_dentry, &st, stat);
		}

		fput(filep);
	}
	unlock_kernel();
	return error;
}
