/*
 *  Function prototypes used by the BSD emulator
 *
 *  Copyright (C) 1994  Mike Jagdis (jaggy@purplet.demon.co.uk)
 *
 * $Id$
 * $Source$
 */

/* Ioctl's have the command encoded in the lower word, and the size of
 * any in or out parameters in the upper word.  The high 3 bits of the
 * upper word are used to encode the in/out status of the parameter.
 * Note that Linux does the same but has the IOC_IN and IOC_OUT values
 * round the other way and uses 0 for IOC_VOID.
 */
#define	BSD_IOCPARM_MASK	0x1fff		/* parameter length, at most 13 bits */
#define	BSD_IOC_VOID	0x20000000	/* no parameters */
#define	BSD_IOC_OUT		0x40000000	/* copy out parameters */
#define	BSD_IOC_IN		0x80000000	/* copy in parameters */
#define	BSD_IOC_INOUT	(BSD_IOC_IN|BSD_IOC_OUT)

#define BSD__IOC(inout,group,num,len) \
	(inout | ((len & BSD_IOCPARM_MASK) << 16) | ((group) << 8) | (num))
#define	BSD__IO(g,n)		BSD__IOC(BSD_IOC_VOID, (g), (n), 0)
#define	BSD__IOR(g,n,t)		BSD__IOC(BSD_IOC_OUT, (g), (n), sizeof(t))
#define	BSD__IOW(g,n,t)		BSD__IOC(BSD_IOC_IN, (g), (n), sizeof(t))
#define	BSD__IOWR(g,n,t)	BSD__IOC(BSD_IOC_INOUT,	(g), (n), sizeof(t))

/* Some SYSV systems exhibit "compatible" BSD ioctls without the bumf. */
#define BSD__IOV(c,d)	(((c) << 8) | (d))



#include <linux/limits.h>


/* Some BSD values have been extended to 64 bit types... */
typedef long long quad;

/* From bsd.c */
extern int bsd_getpagesize(void);
extern int bsd_geteuid(void);
extern int bsd_getegid(void);
extern int bsd_sbrk(unsigned long n);
extern int bsd_getdtablesize(void);
extern int bsd_killpg(int pgrp, int sig);
extern int bsd_setegid(int egid);
extern int bsd_seteuid(int euid);
extern int bsd_open(const char * fname, int flag, int mode);
extern int bsd_fcntl(struct pt_regs *regs);
extern int bsd_ioctl(struct pt_regs *regs);

/* From bsdioctl.c */
extern int bsd_ioctl_termios(int fd, unsigned int func, void *arg);

/* From bsdsignal.c */
#define BSD_SA_ONSTACK		0x0001
#define BSD_SA_RESTART		0x0002
#define BSD_SA_NOCLDSTOP	0x0004
struct bsd_sigaction {
	void (*sa_handler)(int);
	unsigned int sa_mask;
	int sa_flags;
};
extern int bsd_sigaction(int bsd_signum, const struct bsd_sigaction *action,
	struct bsd_sigaction *oldaction);
extern int bsd_sigprocmask(int how, unsigned long bsdnset, unsigned long *bsdoset);
extern int bsd_sigpending(unsigned long *set);

/* From bsdsocket.c */
extern int bsd_connect(struct pt_regs *regs);

/* From bsdstat.c */
struct bsd_stat {
	unsigned short	st_dev;
	unsigned long	st_ino;
	unsigned short	st_mode;
	unsigned short	st_nlink;
	unsigned short	st_uid;
	unsigned short	st_gid;
	unsigned short	st_rdev;
	unsigned long	st_size;
	unsigned long	st_atime;
	unsigned long	st_spare1;
	unsigned long	st_mtime;
	unsigned long	st_spare2;
	unsigned long	st_ctime;
	unsigned long	st_spare3;
	unsigned long	st_blksize;
	unsigned long	st_blocks;
	unsigned long	st_flags;
	unsigned long	st_gen;
};
extern int bsd_stat(char *filename, struct bsd_stat *st);
extern int bsd_lstat(char *filename, struct bsd_stat *st);
extern int bsd_fstat(unsigned int fd, struct bsd_stat *st);


#define MNAMELEN 90	/* length of buffer for returned name */

struct bsd_statfs {
	short	f_type;			/* type */
	short	f_flags;		/* copy of mount flags */
	long	f_fsize;		/* fundamental file system block size */
	long	f_bsize;		/* optimal transfer block size */
	long	f_blocks;		/* total data blocks in file system */
	long	f_bfree;		/* free blocks in fs */
	long	f_bavail;		/* free blocks avail to non-superuser */
	long	f_files;		/* total file nodes in file system */
	long	f_ffree;		/* free file nodes in fs */
	quad	f_fsid;			/* file system id */
	long	f_spare[9];		/* spare for later */
	char	f_mntonname[MNAMELEN];	/* directory on which mounted */
	char	f_mntfromname[MNAMELEN];/* mounted filesystem */
};
extern int bsd_statfs(const char *path, struct bsd_statfs *buf);
extern int bsd_fstatfs(unsigned int fd, struct bsd_statfs *buf);


#define MAXNAMELEN	NAME_MAX

struct bsd_dirent {
	unsigned long	d_fileno;
	unsigned short	d_reclen;
	unsigned short	d_namlen;
	char		d_name[MAXNAMELEN+1];
};
extern int bsd_getdirentries(int fd, char *buf, int nbytes, char *end_posn);
