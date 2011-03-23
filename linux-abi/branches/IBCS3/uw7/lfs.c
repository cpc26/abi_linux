/*
 * Copyright (c) 1999 Tigran Aivazian.
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
 * Support for the UnixWare 7.x LFS (Large File Summit) syscalls.
 */
#include "../include/util/i386_std.h"
#include <linux/sched.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/statfs.h>
#include <linux/kernel.h>
//#include <linux/unistd.h>
#include <linux/syscalls.h>
#include <linux/namei.h>
#include <asm/uaccess.h>

#include "../include/uw7/resource.h"
#include "../include/uw7/statvfs.h"
#include "../include/util/trace.h"


/*
 * The UnixWare 7 truncate64/fruncate64 syscalls are the same as in
 * Linux, but we can't easily handle long long syscall parameters for
 * lcall7, thus we have to fake two 32bit arguments instead.
 *
 * XXX: if do_sys_truncate/do_sys_ftruncate in fs/open.c were exported
 *	we could get rid of one function call.
 */
int
uw7_truncate64(const char *filename, u_int len, u_int len_hi)
{
int err;
#ifdef CONFIG_65BIT
	err = SYS(truncate,filename, (len | (loff_t)len_hi << 32));
#else
	err = SYS(truncate64,filename, (len | (loff_t)len_hi << 32));
#endif
 return err;
}

int
uw7_ftruncate64(int fd, u_int len, u_int len_hi)
{
int err;
#ifdef CONFIG_65BIT
	err = SYS(ftruncate,fd, (len | (loff_t)len_hi << 32));
#else
	err = SYS(ftruncate64,fd, (len | (loff_t)len_hi << 32));
#endif
return err;
}

/*
 * The SVR4 statvfs is basically our statfs, but of course only
 * basically ....
 */
static int
cp_uw7_statvfs64(struct super_block *sbp, struct kstatfs *srcp,
		struct uw7_statvfs64 *dstp)
{
	struct uw7_statvfs64	tmp;

	memset(&tmp, 0, sizeof(struct uw7_statvfs64));

	tmp.f_bsize  = srcp->f_bsize;
	tmp.f_frsize = srcp->f_bsize;
	tmp.f_blocks = srcp->f_blocks;
	tmp.f_bfree  = srcp->f_bfree;
	tmp.f_bavail = srcp->f_bavail;
	tmp.f_files  = srcp->f_files;
	tmp.f_ffree  = srcp->f_ffree;
	tmp.f_favail = srcp->f_ffree;
	tmp.f_fsid   = sbp->s_dev;

	strcpy(tmp.f_basetype, sbp->s_type->name);

	tmp.f_namemax = srcp->f_namelen;

	if (copy_to_user(dstp, &tmp, sizeof(struct uw7_statvfs64)))
		return -EFAULT;
	return 0;
}

int
uw7_statvfs64(char *filename, struct uw7_statvfs64 *stvfsp)
{
#if _KSL > 26
	struct path             path;
#else
	struct nameidata	nd;
#endif
	int			error;

#if _KSL > 26
	error = user_path(filename, &path);
#else
	error = user_path_walk(filename, &nd);
#endif
	if (!error) {
#if _KSL > 24
#if _KSL > 26
		struct super_block	*sbp = path.dentry->d_inode->i_sb;
#else
		struct super_block	*sbp = nd.path.dentry->d_inode->i_sb;
#endif
#else
		struct super_block	*sbp = nd.dentry->d_inode->i_sb;
#endif
		struct kstatfs		tmp;

#if _KSL < 18
		error = vfs_statfs(sbp, &tmp);
#else
		error = vfs_statfs(sbp->s_root, &tmp);
#endif
		if (!error && cp_uw7_statvfs64(sbp, &tmp, stvfsp))
			error = -EFAULT;
#if _KSL < 25
		path_release(&nd);
#else
#if _KSL > 26
		path_put(&path);
#else
		path_put(&nd.path);
#endif
#endif
	}

	return (error);
}


int
uw7_fstatvfs64(int fd, struct uw7_statvfs64 *stvfsp)
{
	struct file		*fp;
	int			error = -EBADF;

	fp = fget(fd);
	if (fp) {
		struct super_block	*sbp = fp->f_dentry->d_inode->i_sb;
		struct kstatfs		tmp;

#if _KSL < 18
		error = vfs_statfs(sbp, &tmp);
#else
		error = vfs_statfs(sbp->s_root, &tmp);
#endif
		if (!error && cp_uw7_statvfs64(sbp, &tmp, stvfsp))
			error = -EFAULT;
		fput(fp);
	}

	return (error);
}

static __inline__ int
uw7_rlim64_to_user(int resource, struct uw7_rlim64 *rlimp)
{
#if _KSL < 10
	struct rlimit		*lxrlim = current->rlim + resource;
#else
	struct rlimit		*lxrlim = current->signal->rlim + resource;
#endif
	struct uw7_rlim64	rlim;

	rlim.rlim_cur = lxrlim->rlim_cur;
	rlim.rlim_max = lxrlim->rlim_max;

	if (copy_to_user(rlimp, &rlim, sizeof(struct uw7_rlim64)))
		return -EFAULT;
	return 0;
}

int
uw7_getrlimit64(int resource, struct uw7_rlim64 *rlimp)
{
	if (resource > ARRAY_SIZE(uw7_to_linux_rlimit))
		return -EINVAL;

	return (uw7_rlim64_to_user(uw7_to_linux_rlimit[resource], rlimp));
}

static __inline__ int
uw7_check_rlimit64(int resource, const struct uw7_rlim64 *rlimp,
		   struct rlimit *lxrlimp)
{
	if ((rlimp->rlim_cur > RLIM_INFINITY) ||
	    (rlimp->rlim_max > RLIM_INFINITY))
		return -EPERM;  /* XXX: actually this is wrong */

	if (((rlimp->rlim_cur > lxrlimp->rlim_max) ||
	     (rlimp->rlim_max > lxrlimp->rlim_max)) &&
	      !capable(CAP_SYS_RESOURCE))
		return -EPERM;

	if (resource == RLIMIT_NOFILE &&
	    (rlimp->rlim_cur > 1024 || rlimp->rlim_max > 1024))
		return -EPERM;

	return 0;
}

int
uw7_setrlimit64(int resource, const struct uw7_rlim64 *rlimp)
{
#if _KSL < 10
	struct rlimit		*lxrlim = current->rlim + resource;
#else
	struct rlimit		*lxrlim = current->signal->rlim + resource;
#endif
	struct uw7_rlim64	rlim;

	if (resource > ARRAY_SIZE(uw7_to_linux_rlimit))
		return -EINVAL;
	if (copy_from_user(&rlim, rlimp, sizeof(struct uw7_rlim64)))
		return -EFAULT;
	if (uw7_check_rlimit64(resource, &rlim, lxrlim))
		return -EPERM;

	/* XXX: this is non-atomic. */
	lxrlim->rlim_cur = rlim.rlim_cur;
	lxrlim->rlim_max = rlim.rlim_max;

	return 0;
}

/*
 * 64bit lseek for UnixWare.
 */
int
uw7_lseek64(int fd, u_int off, u_int off_hi, int orig)
{
	loff_t			result;
	int			err;

	if (off_hi == (u_int)-1)
		off_hi = 0;

#ifdef CONFIG_65BIT
	err  = SYS(lseek,fd, ( (long)off_hi << 32) + (long)off, orig);
	result = err;
#else
	err  = SYS(_llseek,fd, (off_t) off_hi, off, &result, orig);
#endif
	if (err)
		return err;
	return (long)result; /* XXX: how does UnixWare return large results? */
}

/*
 * The UnixWare 7 pread64/pwrite64 syscalls are the same as in Linux,
 * but we can't easily handle long long syscall parameters for lcall7,
 * thus we have to fake two 32bit arguments instead.
 */
ssize_t
uw7_pread64(int fd, char *bufp, int count, u_int pos, u_int pos_hi)
{
int err;
	err = SYS(pread64,fd, bufp, count, (pos | (loff_t)pos_hi << 32)); return err;
}

ssize_t
uw7_pwrite64(int fd, char *bufp, int count, u_int pos, u_int pos_hi)
{
int err;
	err = SYS(pwrite64,fd, bufp, count, (pos | (loff_t)pos_hi << 32)); return err;
}

/*
 * Unlike Linux UnixWare 7 does not simply add O_LARGEFILE to flags in
 * libc, so we have to do it.
 * We call sys_open directly as sys_creat is just yet another wrapper.
 */
#define UW7_CREAT64_FLAGS \
	(O_LARGEFILE | O_CREAT | O_WRONLY | O_TRUNC)
int
uw7_creat64(const char *filename, int mode)
{
int err;
	err = SYS(open,filename, UW7_CREAT64_FLAGS, mode); return err;
}
