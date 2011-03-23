/*
 * This is straight from linux/fs/stat.c.
 */
#ifndef _ABI_UTIL_REVALIDATE_H
#define _ABI_UTIL_REVALIDATE_H

//#ident "%W% %G%"

#include <linux/fs.h>

#ifndef CONFIG_ABI_REVALIDATE
static __inline int do_revalidate(struct dentry *dentry) { return 0; }
#else
/* LINUXABI_TODO */
/*
 * This is required for proper NFS attribute caching (so it says there).
 * Maybe the kernel should export it - but it is basically simple...
 */
static __inline int
do_revalidate(struct dentry *dentry)
{
	struct inode *inode = dentry->d_inode;
        struct iattr attr;
        int error;

        attr.ia_valid = 0; /* setup an empty flag for the inode attribute change */
#if _KSL > 15
        mutex_lock(&inode->i_mutex);
#else
	down(&inode->i_sem);
#endif
#if _KSL > 21 && ( defined(CONFIG_SECURITY_APPARMOR) || defined(CONFIG_SECURITY_APPARMOR_MODULE) )
        error = notify_change(dentry,NULL,&attr);
#else
        error = notify_change(dentry,&attr);
#endif

#if _KSL > 15
        mutex_unlock(&inode->i_mutex);
#else
	up(&inode->i_sem);
#endif

	return error;
}

#endif /* CONFIG_ABI_REVALIDATE */
#endif /* _ABI_UTIL_REVALIDATE_H */
