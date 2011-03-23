/*
 *	abi/uw7/access.c - support for UnixWare access(2) system call.
 *
 *	We handle the non-POSIX EFF_ONLY_OK/EX_OK flags.
 *	This software is under GPL.
 */

#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/smp_lock.h>

#include <asm/uaccess.h>

#include <abi/abi.h>
#include <abi/uw7.h>

#undef DEBUG

#ifdef DEBUG
#define DBG(x...)	printk(x)
#else
#define DBG(x...)
#endif

#define UW7_R_OK	004
#define UW7_W_OK	002
#define UW7_X_OK	001
#define UW7_F_OK	000
#define UW7_EFF_ONLY_OK	010
#define UW7_EX_OK	020

#define UW7_MODE_MSK	(UW7_R_OK|UW7_W_OK|UW7_X_OK|UW7_F_OK|UW7_EFF_ONLY_OK|UW7_EX_OK)

int uw7_access(char * filename, int mode)
{
	int error;
	struct dentry * dentry;

	DBG(KERN_ERR "UW7[%d]: access(%p,%o)\n", current->pid, filename, mode);

	if (mode & ~UW7_MODE_MSK)
		return -EINVAL;

	if (mode & UW7_EX_OK) {
		lock_kernel();
		dentry = namei(filename);
		error = PTR_ERR(dentry);
		if (!IS_ERR(error)) {
			error = do_revalidate(dentry);
			if (error) {
				dput(dentry);
				unlock_kernel();
				return -EIO;
			}
			if (!S_ISREG(dentry->d_inode->i_mode)) {
				dput(dentry);
				unlock_kernel();
				return -EACCES;
			}
			dput(dentry);
		}
		unlock_kernel();
		mode &= ~UW7_EX_OK;
		mode |= UW7_X_OK;
	}
	if (mode & UW7_EFF_ONLY_OK) {
		uid_t old_uid = current->uid, old_gid = current->gid;

		current->uid = current->euid;
		current->gid = current->egid;
		mode &= ~UW7_EFF_ONLY_OK;
        	error =  SYS(access)(filename, mode);
		current->uid = old_uid;
		current->gid = old_gid;
	} else
        	error = SYS(access)(filename, mode);

	return error;
}
