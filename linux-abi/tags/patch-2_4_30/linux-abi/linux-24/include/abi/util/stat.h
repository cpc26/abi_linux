/*
 * Mostly ripped from Al Viro's stat-a-AC9-10 patch, 2001 Christoph Hellwig.
 */
#ifndef _ABI_UTIL_STAT_H
#define _ABI_UTIL_STAT_H

#ident "%W% %G%"

struct kstat {
	ino_t		ino;
	atomic_t	count;
	dev_t		dev;
	umode_t		mode;
	nlink_t		nlink;
	uid_t		uid;
	gid_t		gid;
	dev_t		rdev;
	loff_t		size;
	time_t		atime;
	time_t		mtime;
	time_t		ctime;
	unsigned long	blksize;
	unsigned long	blocks;
};

extern int vfs_stat(char *, struct kstat *);
extern int vfs_lstat(char *, struct kstat *);
extern int vfs_fstat(int, struct kstat *);
extern ino_t short_inode_map(ino_t);

#endif /* _ABI_UTIL_STAT_H */
