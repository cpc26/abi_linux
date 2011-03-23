/*
 *  abi/svr4_common/open.c
 *
 *  Copyright (C) 1993  Joe Portman (baron@hebron.connected.com)
 *  Copyright (C) 1993, 1994  Drew Sullivan (re-worked for iBCS2)
 *
 * $Id$
 * $Source$
 */

/* Keep track of which struct definition we really want */

#include <linux/config.h>
#define __NO_VERSION__
#include <linux/module.h>
#include <linux/version.h>

#include <asm/uaccess.h>

#include <linux/vfs.h>
#include <linux/types.h>
#include <linux/utime.h>
#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/stat.h>
#include <linux/string.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/signal.h>
#include <linux/tty.h>
#include <linux/time.h>
#include <linux/malloc.h>
#include <linux/un.h>
#include <linux/file.h>
#include <linux/smp_lock.h>

#include <asm/bitops.h>

#include <abi/abi.h>
#include <abi/compat.h>

#ifdef __NR_getdents
#include <linux/dirent.h>
#endif

#ifdef CONFIG_ABI_TRACE
#include <abi/trace.h>
#endif


#ifdef __cplusplus
extern "C" 
#endif

/* ISC (at least) assumes O_CREAT if O_TRUNC is given. This is emulated
 * here but is it correct for iBCS in general? Do we care?
 */
unsigned short fl_ibcs_to_linux[] = {
	0x0001, 0x0002, 0x0800, 0x0400, 0x1000, 0x0000, 0x0000, 0x0800,
	0x0040, 0x0240, 0x0080, 0x0100, 0x0000, 0x0000, 0x0000, 0x0000
};

EXPORT_SYMBOL(fl_ibcs_to_linux);

unsigned short fl_linux_to_ibcs[] = {
	0x0001, 0x0002, 0x0000, 0x0000, 0x0000, 0x0000, 0x0100, 0x0400,
	0x0800, 0x0200, 0x0008, 0x0004, 0x0010, 0x0000, 0x0000, 0x0000
};


int svr4_statfs(const char * path, struct ibcs_statfs * buf, int len, int fstype)
{
	struct ibcs_statfs ibcsstat;

	if (len > (int)sizeof(struct ibcs_statfs))
		return -EINVAL;

	if (!fstype) {
		struct dentry *dentry;
		struct statfs lxstat;
		int error;
		mm_segment_t old_fs;

		lock_kernel();
		dentry = namei(path);
		error = PTR_ERR(dentry);
		if (!IS_ERR(dentry)) {
			struct inode *inode = dentry->d_inode;

			error = -ENOSYS;
			if (inode->i_sb->s_op->statfs) {
				old_fs = get_fs();
				set_fs(get_ds());
				error = inode->i_sb->s_op->statfs(inode->i_sb, &lxstat);
				set_fs(old_fs);

				if (!error) {
					ibcsstat.f_type = lxstat.f_type;
					ibcsstat.f_bsize = lxstat.f_bsize;
					ibcsstat.f_frsize = 0;
					ibcsstat.f_blocks = lxstat.f_blocks;
					ibcsstat.f_bfree = lxstat.f_bfree;
					ibcsstat.f_files = lxstat.f_files;
					ibcsstat.f_ffree = lxstat.f_ffree;
					memset(ibcsstat.f_fname, 0, sizeof(ibcsstat.f_fname));
					memset(ibcsstat.f_fpack, 0, sizeof(ibcsstat.f_fpack));

					/* Finally, copy it to the user's buffer */
					if (copy_to_user(buf, &ibcsstat, len))
						error = -EFAULT;
				}
			}
			dput(dentry);
		}
		unlock_kernel();
		return error;
	}

	/* Linux can't stat unmounted filesystems so we
	 * simply lie and claim 100MB of 1GB is free. Sorry.
	 */
	ibcsstat.f_bsize = 1024;
	ibcsstat.f_frsize = 0;
	ibcsstat.f_blocks = 1024 * 1024;	/* 1GB */
	ibcsstat.f_bfree = 100 * 1024;		/* 100MB */
	ibcsstat.f_files = 60000;
	ibcsstat.f_ffree = 50000;
	memset(ibcsstat.f_fname, 0, sizeof(ibcsstat.f_fname));
	memset(ibcsstat.f_fpack, 0, sizeof(ibcsstat.f_fpack));

	/* Finally, copy it to the user's buffer */
	return copy_to_user(buf, &ibcsstat, len) ? -EFAULT : 0;
}

EXPORT_SYMBOL(svr4_statfs);

#ifdef __cplusplus
extern "C" 
#endif
int svr4_fstatfs(unsigned int fd, struct ibcs_statfs * buf, int len, int fstype)
{
	struct ibcs_statfs ibcsstat;

	if (len > (int)sizeof(struct ibcs_statfs))
		return -EINVAL;

	if (!fstype) {
		struct file * file;
		struct inode * inode;
		struct dentry * dentry;
		struct super_block * sb;
		int error;
		struct statfs lxstat;
		mm_segment_t old_fs;

		lock_kernel();
		error = -EBADF;
		file = fget(fd);
		if (!file)
			goto out;
		error = -ENOENT;
		if (!(dentry = file->f_dentry))
			goto out_putf;
		if (!(inode = dentry->d_inode))
			goto out_putf;
		error = -ENODEV;
		if (!(sb = inode->i_sb))
			goto out_putf;
		error = -ENOSYS;
		if (sb->s_op->statfs) {
			old_fs = get_fs();
			set_fs(get_ds());
			error = sb->s_op->statfs(sb, &lxstat);
			set_fs(old_fs);

			if (!error) {
				ibcsstat.f_type = lxstat.f_type;
				ibcsstat.f_bsize = lxstat.f_bsize;
				ibcsstat.f_frsize = 0;
				ibcsstat.f_blocks = lxstat.f_blocks;
				ibcsstat.f_bfree = lxstat.f_bfree;
				ibcsstat.f_files = lxstat.f_files;
				ibcsstat.f_ffree = lxstat.f_ffree;
				memset(ibcsstat.f_fname, 0, sizeof(ibcsstat.f_fname));
				memset(ibcsstat.f_fpack, 0, sizeof(ibcsstat.f_fpack));

				/* Finally, copy it to the user's buffer */
				if (copy_to_user(buf, &ibcsstat, len))
					error = -EFAULT;
			}
		}
out_putf:
		fput(file);
out:
		unlock_kernel();
		return error;
	}

	/* Linux can't stat unmounted filesystems so we
	 * simply lie and claim 100MB is of 1GB free. Sorry.
	 */
	ibcsstat.f_bsize = 1024;
	ibcsstat.f_frsize = 0;
	ibcsstat.f_blocks = 1024 * 1024;	/* 1GB */
	ibcsstat.f_bfree = 100 * 1024;		/* 100MB */
	ibcsstat.f_files = 60000;
	ibcsstat.f_ffree = 50000;
	memset(ibcsstat.f_fname, 0, sizeof(ibcsstat.f_fname));
	memset(ibcsstat.f_fpack, 0, sizeof(ibcsstat.f_fpack));

	/* Finally, copy it to the user's buffer */
	return copy_to_user(buf, &ibcsstat, len) ? -EFAULT : 0;
}

EXPORT_SYMBOL(svr4_fstatfs);


int svr4_open(const char *fname, int flag, int mode)
{
#ifdef __sparc__
	return SYS(open)(fname, map_flags(flag, fl_ibcs_to_linux), mode);
#else
	int error, fd, args[3];
	struct file *file;
	mm_segment_t old_fs;
	char *p;
	struct sockaddr_un addr;

	fd = SYS(open)(fname, map_flags(flag, fl_ibcs_to_linux), mode);
	if (fd < 0)
		return fd;

	/* Sometimes a program may open a pathname which it expects
	 * to be a named pipe (or STREAMS named pipe) when the
	 * Linux domain equivalent is a Unix domain socket. (e.g.
	 * UnixWare uses a STREAMS named pipe /dev/X/Nserver.0 for
	 * X :0 but Linux uses a Unix domain socket /tmp/.X11-unix/X0)
	 * It isn't enough just to make the symlink because you cannot
	 * open() a socket and read/write it. If we spot the error we can
	 * switch to socket(), connect() and things will likely work
	 * as expected however.
	 */
	file = fget(fd);
	if (!file)
		return fd; /* Huh?!? */
	if (!S_ISSOCK(file->f_dentry->d_inode->i_mode)) {
		fput(file);
		return fd;
	}
	fput(file);

	SYS(close)(fd);
	args[0] = AF_UNIX;
	args[1] = SOCK_STREAM;
	args[2] = 0;
	old_fs = get_fs();
	set_fs(get_ds());
	fd = SYS(socketcall)(SYS_SOCKET, args);
	set_fs(old_fs);
	if (fd < 0)
		return fd;

	p = getname(fname);
	if (IS_ERR(p)) {
		SYS(close)(fd);
		return PTR_ERR(p);
	}
	if (strlen(p) >= UNIX_PATH_MAX) {
		putname(p);
		SYS(close)(fd);
		return -E2BIG;
	}
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, p);
	putname(p);

	args[0] = fd;
	args[1] = (int)&addr;
	args[2] = sizeof(struct sockaddr_un);
	set_fs(get_ds());
	error = SYS(socketcall)(SYS_CONNECT, args);
	set_fs(old_fs);
	if (error) {
		SYS(close)(fd);
		return error;
	}

	return fd;
#endif
}

EXPORT_SYMBOL(svr4_open);


/* If/when the readdir function is changed to read multiple entries
 * at once this should be updated to take advantage of the fact.
 *
 * N.B. For Linux the reclen in a dirent is the number of characters
 * in the filename, for SCO (at least) reclen is the total size of
 * the particular dirent rounded up to the next multiple of 4. The SCO
 * behaviour is faithfully emulated here.
 *
 * XXXX
 * We don't truncate long filenames at all when copying. If we meet a
 * long filename and the buffer supplied by the application simply isn't
 * big enough to hold it we'll return without filling the buffer (i.e
 * return 0). The application will see this as a (premature) end of
 * directory. Is there a work around for this at all???
 */
int svr4_getdents(int fd, char *buf, int nbytes)
{
	int error, here, posn, reclen;
	struct file *file;
	struct dirent *d;
	mm_segment_t old_fs;

	error = verify_area(VERIFY_WRITE, buf, nbytes);
	if (error)
		return error;

	/* Check the file handle here. This is so we can access the current
	 * position in the file structure safely without a tedious call
	 * to sys_lseek that does nothing useful.
	 */
	file = fget(fd);
	if (!file)
		return -EBADF;

	d = (struct dirent *)__get_free_page(GFP_KERNEL);
	if (!d) {
		fput(file);
		return -ENOMEM;
	}

	error = posn = reclen = 0;
	while (posn + reclen < nbytes) {
		/* Save the current position and get another dirent */
		here = file->f_pos;
		old_fs = get_fs();
		set_fs (get_ds());
		error = SYS(readdir)(fd, d, 1);
		set_fs(old_fs);
		if (error <= 0)
			break;

		/* If it'll fit in the buffer save it otherwise back up
		 * so it is read next time around.
		 * Oh, if we're at the beginning of the buffer there's
		 * no chance that this entry will ever fit so don't
		 * copy it and don't back off - we'll just pretend it
		 * isn't here...
		 */
		reclen = (sizeof(long) + sizeof(off_t)
			+ sizeof(unsigned short) + d->d_reclen + 1
			+ 3) & (~3);
		if (posn + reclen <= nbytes) {
			if (current->personality & SHORT_INODE) {
				/* read() on a directory only handles
				 * short inodes but cannot use 0 as that
				 * indicates an empty directory slot.
				 * Therefore stat() must also fold
				 * inode numbers avoiding 0. Which in
				 * turn means that getdents() must fold
				 * inodes avoiding 0 - if the program
				 * was built in a short inode environment.
				 * If we have short inodes in the dirent
				 * we also have a two byte pad so we
				 * can let the high word fall in the pad.
				 * This makes it a little more robust if
				 * we guessed the inode size wrong.
				 */
				if (!((unsigned long)d->d_ino & 0xffff))
					d->d_ino = 0xfffffffe;
			}
			d->d_reclen = reclen;
			d->d_off = file->f_pos;
			copy_to_user(buf+posn, d, reclen);
			posn += reclen;
		} else if (posn) {
			SYS(lseek)(fd, here, 0);
		} /* else posn == 0 */
	}

	/* Loose the intermediate buffer. */
	free_page((unsigned long)d);

	fput(file);

	/* If we've put something in the buffer return the byte count
	 * otherwise return the error status.
	 */
	return ((posn > 0) ? posn : error);
}

EXPORT_SYMBOL(svr4_getdents);

struct ibcs_flock {
	short l_type;	/* numbers don't match */
	short l_whence;
	off_t l_start;
	off_t l_len;	/* 0 means to end of file */
	short l_sysid;
	short l_pid;
};


int svr4_fcntl(struct pt_regs *regs)
{
	int arg1, arg2, arg3;
	int error, retval;

#ifndef __sparc__
	error = verify_area(VERIFY_READ,
			((unsigned long *)regs->esp)+1,
			3*sizeof(long));
	if (error)
		return error;
#endif /* __sparc__ */
	arg1 = get_syscall_parameter (regs, 0);
	arg2 = get_syscall_parameter (regs, 1);
	arg3 = get_syscall_parameter (regs, 2);

	switch (arg2) {
		/* These match the Linux commands. */
		case 0: /* F_DUPFD */
		case 1: /* F_GETFD */
		case 2: /* F_SETFD */
			return SYS(fcntl)(arg1, arg2, arg3);

		/* The iBCS flags don't match Linux flags. */
		case 3: /* F_GETFL */
			return map_flags(SYS(fcntl)(arg1, arg2, arg3),
					fl_linux_to_ibcs);
		case 4: /* F_SETFL */
			arg3 = map_flags(arg3, fl_ibcs_to_linux);
			return SYS(fcntl)(arg1, arg2, arg3);

		/* The lock stucture is different. */
		case 14: /* F_GETLK SVR4 */
			arg2 = 5;
			/* fall through */
		case 5: /* F_GETLK */
		case 6: /* F_SETLK */
		case 7: /* F_SETLKW */
		{
			struct ibcs_flock fl;
			struct flock l_fl;
			mm_segment_t old_fs;

			error = verify_area(VERIFY_WRITE, (void *)arg3,
					sizeof(fl));
			if (error)
				return error;
			error = copy_from_user(&fl, (void *)arg3, sizeof(fl));
			if (error)
				return -EFAULT;

			l_fl.l_type = fl.l_type - 1;
			l_fl.l_whence = fl.l_whence;
			l_fl.l_start = fl.l_start;
			l_fl.l_len = fl.l_len;
			l_fl.l_pid = fl.l_pid;
#ifdef CONFIG_ABI_TRACE
			if ((ibcs_trace & TRACE_API) || ibcs_func_p->trace) {

				printk (KERN_DEBUG "iBCS: lock l_type: %d l_whence: %d l_start: %lu l_len: %lu l_sysid: %d l_pid: %d\n",
							fl.l_type,
							fl.l_whence,
							fl.l_start,
							fl.l_len,
							fl.l_sysid,
							fl.l_pid);

			}
#endif
			old_fs = get_fs();
			set_fs(get_ds());
			retval = SYS(fcntl)(arg1, arg2, &l_fl);
			set_fs(old_fs);

			if (!retval) {
				fl.l_type = l_fl.l_type + 1;
				fl.l_whence = l_fl.l_whence;
				fl.l_start = l_fl.l_start;
				fl.l_len = l_fl.l_len;
				fl.l_sysid = 0;
				fl.l_pid = l_fl.l_pid;
				/* This should not fail... */
				copy_to_user((void *)arg3, &fl, sizeof(fl));
			}

			return retval;
		}

		case 10: /* F_ALLOCSP */
			/* Extend allocation for specified portion of file. */
		case 11: /* F_FREESP */
			/* Free a portion of a file. */
			return 0;

		/* These are intended to support the Xenix chsize() and
		 * rdchk() system calls. I don't know if these may be
		 * generated by applications or not.
		 */
		case 0x6000: /* F_CHSIZE */
			return SYS(ftruncate)(arg1, arg3);
#ifndef __sparc__
		case 0x6001: /* F_RDCHK */
			return xnx_rdchk(arg1);
#endif /* __sparc__ */

#ifdef CONFIG_ABI_IBCS_SCO
		/* This could be SCO's get highest fd open if the fd we
		 * are called on is -1 otherwise it could be F_CHKFL.
		 */
		case  8: /* F_GETHFDO */
			if (arg1 == -1)
				return find_first_zero_bit(OPEN_FDS_ADDR, FDS_TABLE_LEN);
			/* else fall through to fail */
#else
		/* The following are defined but reserved and unknown. */
		case  8: /* F_CHKFL */
#endif

		/* These are made from the Xenix locking() system call.
		 * According to available documentation these would
		 * never be generated by an application - only by the
		 * kernel Xenix support.
		 */
		case 0x6300: /* F_LK_UNLCK */
		case 0x7200: /* F_LK_LOCK */
		case 0x6200: /* F_LK_NBLCK */
		case 0x7100: /* F_LK_RLCK */
		case 0x6100: /* F_LK_NBRLCK */

		default:
#ifdef CONFIG_ABI_TRACE
			if ((ibcs_trace & TRACE_API) || ibcs_func_p->trace) {
					printk(KERN_ERR "iBCS: unsupported fcntl 0x%lx, arg 0x%lx\n",
					(unsigned long)arg2, (unsigned long)arg3);
			}
#endif
			return -EINVAL;
			break;
	}
}

EXPORT_SYMBOL(svr4_fcntl);
