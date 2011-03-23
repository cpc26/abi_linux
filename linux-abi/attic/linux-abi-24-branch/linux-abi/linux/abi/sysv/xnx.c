/*
 *     abi/svr4_common/xnx.c
 *
 *  Copyright (C) 1993,1994  Drew Sullivan
 *  Copyright (C) 1994,1995-1996  Mike Jagdis
 *
 *  This contains the set of Xenix syscalls that are used by SCO binaries
 *
 * $Id$
 * $Source$
 */

#include <linux/config.h>
#define __NO_VERSION__
#include <linux/module.h>
#include <linux/version.h>

#include <asm/uaccess.h>

#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/stddef.h>
#include <linux/unistd.h>
#include <linux/ptrace.h>
#include <linux/fcntl.h>
#include <linux/time.h>
#include <linux/signal.h>

#include <asm/system.h>
#include <linux/fs.h>
#include <linux/sys.h>
#include <linux/termios.h>
#include <linux/limits.h>
#include <linux/vfs.h>

#include <abi/abi.h>

#ifdef CONFIG_ABI_TRACE
#include <abi/trace.h>
#endif

#include <abi/xnx.h>


#define XF_UNLCK	0
#define XF_WRLCK	1
#define XF_RDLCK	3


struct ibcs_flock {
	short l_type;
	short l_whence;
	off_t l_start;
	off_t l_len;
	short l_sysid;
	short l_pid;
};


/* locking() requires mandatory locking. Processes that attempt to
 * read or write a region locked with locking() are required to block.
 * You need to build a kernel with mandatory locking support and set
 * the permissions on the required file to setgid, no group execute.
 */
int xnx_locking(int fd, int mode, unsigned long size)
{
	struct flock fl;
	mm_segment_t old_fs;
	int error;

	if ((mode < 0 || mode > 7) && mode != 20) {
#ifdef CONFIG_ABI_TRACE
		if ((ibcs_trace & TRACE_API) || ibcs_func_p->trace) {
	    		printk(KERN_ERR
				"iBCS: unsupported locking() mode=0x%x\n",
	    			mode);
		}
#endif
		return -EINVAL;
	}

	/* Modes 5, 6 & 7 are very like the fcntl mechanism but
	 * we can't just punt to that because the type values are
	 * different.
	 */
	if (mode > 4 && mode < 8) {
		struct ibcs_flock *ifl = (struct ibcs_flock *)size;
		short t;

		error = verify_area(VERIFY_READ, ifl, sizeof(*ifl));
		if (error)
			return error;

		get_user(t, &ifl->l_type);
		switch (t) {
			case XF_UNLCK:	t = F_UNLCK; break;
			case XF_WRLCK:	t = F_WRLCK; break;
			case XF_RDLCK:	t = F_RDLCK; break;
			default:	return -EINVAL;
		}
		put_user(t, &ifl->l_type);

		error = SYS(fcntl)(fd, mode, ifl);

		get_user(t, &ifl->l_type);
		switch (t) {
			case F_UNLCK:	t = XF_UNLCK; break;
			case F_WRLCK:	t = XF_WRLCK; break;
			case F_RDLCK:	t = XF_RDLCK; break;
		}
		put_user(t, &ifl->l_type);

		get_user(t, &ifl->l_sysid);
		put_user(t, &ifl->l_pid);
		put_user(0, &ifl->l_sysid);
		return error;
	}

	fl.l_type = (mode == 0 ? F_UNLCK
			: ((mode <= 2 || mode == 20) ? F_WRLCK
			: F_RDLCK));
	fl.l_whence = 1;
	fl.l_start = 0;
	fl.l_len = size;

	old_fs = get_fs();
	set_fs (get_ds());
	error = SYS(fcntl)(fd,
			mode == 5 ? F_GETLK
				: (!(mode % 2) ? F_SETLK : F_SETLKW),
			&fl);
	set_fs(old_fs);
	return error;
}


int xnx_creatsem(char *sem_name, int mode) {
	return -EPERM;
}

int xnx_opensem(char *sem_name) {
	return -EPERM;
}

int xnx_sigsem(int sem_num) {
	return -EPERM;
}

int xnx_waitsem(int sem_num) {
	return -EPERM;
}

int xnx_nbwaitsem(int sem_num) {
	return -EPERM;
}

/* Check if input is available */
int xnx_rdchk(int fd) {
	int error, nbytes;
	mm_segment_t old_fs;

	old_fs = get_fs();
	set_fs (get_ds());
	error = SYS(ioctl)(fd, FIONREAD, &nbytes);
	set_fs(old_fs);

	if (error < 0) return error;
	return nbytes ? 1 : 0;
}

/* Linux has a stub sys_ftime. Perhaps this should be there? On the other
 * hand it's an old call that probably shouldn't be used by most modern
 * applications so perhaps it's better here where it needn't bloat the
 * base kernel.
 */
int xnx_ftime(struct timeb * tp) {
	struct timeval tv;
	struct timezone tz;
	int error;
	mm_segment_t old_fs;

	error = verify_area(VERIFY_WRITE, tp, sizeof(struct timeb));
	if (error)
		return error;

	old_fs = get_fs();
	set_fs (get_ds());
	error = SYS(gettimeofday)(&tv, &tz);
	set_fs(old_fs);
	if (error)
		return error;

	put_user(tv.tv_sec, &tp->time);
	put_user((unsigned short)(tv.tv_usec/1000), &tp->millitm);
	put_user((short)tz.tz_minuteswest, &tp->timezone);
	put_user((short)tz.tz_dsttime, &tp->dstflag);

	return 0;
}

/* go to sleep for period milliseconds */
int xnx_nap(long period)
{
	__sighandler_t old_handler;
	struct itimerval it;
	struct timeval tv1, tv2;
	struct timezone tz;
	mm_segment_t old_fs;

	if (!period)
		return 0;

	it.it_interval.tv_sec = 0;
	it.it_interval.tv_usec = 0;
	it.it_value.tv_sec = 0;
	it.it_value.tv_usec = period * 1000;
	old_fs = get_fs();
	set_fs (get_ds());
	SYS(gettimeofday)(&tv1, &tz);
	spin_lock_irq(&current->sig[SIGALRM-1].siglock);
	old_handler = current->sig->action[SIGALRM-1].sa.sa_handler;
	current->sig->action[SIGALRM-1].sa.sa_handler = SIG_DFL;
	spin_unlock_irq(&current->sig[SIGALRM-1].siglock);
	SYS(setitimer)(ITIMER_REAL, &it, NULL);
	SYS(pause)();
	spin_lock_irq(&current->sig[SIGALRM-1].siglock);
	current->sig->action[SIGALRM-1].sa.sa_handler = old_handler;
	spin_unlock_irq(&current->sig[SIGALRM-1].siglock);
	SYS(gettimeofday)(&tv2, &tz);
	set_fs(old_fs);

	deactivate_signal(current, SIGALRM);

	return (signal_pending(current)
		? -EINTR
		: ((tv2.tv_sec - tv1.tv_sec) * 1000000
			+ (tv2.tv_usec - tv1.tv_usec)) / 1000);
}

int xnx_sdget(char *path, int flags, long size, int mode) {
	return -EPERM;
}

int xnx_sdfree(char* addr) {
	return -EPERM;
}

int xnx_sdenter(char *addr, int flags) {
	return -EPERM;
}

int xnx_sdleave(char *addr) {
	return -EPERM;
}

int xnx_sdgetv(char *addr) {
	return -EPERM;
}

int xnx_sdwaitv(char *addr, int vnum) {
	return -EPERM;
}

/* This allows processes to be allowed to exceed available swap. The man
 * page isn't too clear  - it seems to suggest Xenix supports physical
 * memory > swap but this doesn't make sense to me? It almost certainly
 * isn't useful for Linux to actually do anything with this - just lie.
 */
int xnx_proctl(int pid, int command, char *arg) {
#define PRHUGEX		1
#define PRNORMEX	2
	return 0;
}

int xnx_execseg(excode_t oldaddr, unsigned size) {
	return -EPERM;
}


int xnx_unexecseg(excode_t addr) {
	return -EPERM;
}

/* eaccess() checks access to the given path using the effective
 * uid/gid rather than the real uid/gid.
 */
int xnx_eaccess(char *path, int mode) {
	unsigned short ouid, ogid;
	int err;

	ouid = current->uid;
	ogid = current->gid;
	current->uid = current->euid;
	current->gid = current->egid;

	err = SYS(access)(path, mode);

	current->uid = ouid;
	current->gid = ogid;

	return err;
}

/* This allows running adb without executing any programs, but disassembly
 * will work fine with that lie.
 */
int xnx_paccess(int pid, int cmd, int offset, int count, char *ptr) {
	return 0;
}

int xnx_sigpending(unsigned long *set) {
	return -EPERM;
}


#define	_PC_LINK_MAX		0
#define	_PC_MAX_CANON		1
#define	_PC_MAX_INPUT		2
#define	_PC_NAME_MAX		3
#define	_PC_PATH_MAX		4
#define	_PC_PIPE_BUF		5
#define	_PC_CHOWN_RESTRICTED	6
#define	_PC_NO_TRUNC		7
#define	_PC_VDISABLE		8

int
xnx_pathconf(char *path, int name)
{
	switch (name) {
		case _PC_LINK_MAX:
			/* Although Linux headers define values on a per
			 * filesystem basis there is no way to access
			 * these without hard coding fs information here
			 * so for now we use a bogus value.
			 */
			return LINK_MAX;

		case _PC_MAX_CANON:
			return MAX_CANON;

		case _PC_MAX_INPUT:
			return MAX_INPUT;

		case _PC_PATH_MAX:
			return PATH_MAX;

		case _PC_PIPE_BUF:
			return PIPE_BUF;

		case _PC_CHOWN_RESTRICTED:
			/* We should really think about this and tell
			 * the truth.
			 */
			return 0;

		case _PC_NO_TRUNC:
			/* Not sure... It could be fs dependent? */
			return 1;

		case _PC_VDISABLE:
			return 1;

		case _PC_NAME_MAX: {
			struct statfs buf;
			char *p;
			int error;
			mm_segment_t old_fs;

			p = getname(path);
			error = PTR_ERR(p);
			if (!IS_ERR(p)) {
				old_fs = get_fs();
				set_fs (get_ds());
				error = SYS(statfs)(p, &buf);
				set_fs(old_fs);
				putname(p);
				if (!error)
					return buf.f_namelen;
			}
			return error;
		}
	}

	return -EINVAL;
}

int
xnx_fpathconf(int fildes, int name)
{
	switch (name) {
		case _PC_LINK_MAX:
			/* Although Linux headers define values on a per
			 * filesystem basis there is no way to access
			 * these without hard coding fs information here
			 * so for now we use a bogus value.
			 */
			return LINK_MAX;

		case _PC_MAX_CANON:
			return MAX_CANON;

		case _PC_MAX_INPUT:
			return MAX_INPUT;

		case _PC_PATH_MAX:
			return PATH_MAX;

		case _PC_PIPE_BUF:
			return PIPE_BUF;

		case _PC_CHOWN_RESTRICTED:
			/* We should really think about this and tell
			 * the truth.
			 */
			return 0;

		case _PC_NO_TRUNC:
			/* Not sure... It could be fs dependent? */
			return 1;

		case _PC_VDISABLE:
			return 1;

		case _PC_NAME_MAX: {
			struct statfs buf;
			int error;
			mm_segment_t old_fs;

			old_fs = get_fs();
			set_fs (get_ds());
			error = SYS(statfs)(fildes, &buf);
			set_fs(old_fs);
			if (!error)
				return buf.f_namelen;
			return error;
		}
	}

	return -EINVAL;
}
