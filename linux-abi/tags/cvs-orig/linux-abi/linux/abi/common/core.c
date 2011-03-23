/*
 *  linux/abi/abi_common/core.c
 *
 *  Copyright (C) 1993  Linus Torvalds
 *
 *   Modified by Eric Youngdale to include all ibcs syscalls.
 *   Re-written by Drew Sullivan to handle lots more of the syscalls correctly.
 *
 * $Id$
 * $Source$
 */

#include <linux/config.h>
#define __NO_VERSION__

#include <linux/module.h>

#include <linux/version.h>

#include <linux/types.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/stat.h>
#include <linux/stddef.h>
#include <linux/unistd.h>
#include <linux/ptrace.h>
#include <linux/fcntl.h>
#include <linux/time.h>

#include <linux/fs.h>
#include <linux/sys.h>
#include <linux/malloc.h>
#include <linux/file.h>

#include <asm/uaccess.h>
#include <asm/system.h>

#include <abi/abi.h>
#include <abi/signal.h>

#ifdef __NR_getdents
#include <linux/dirent.h>
#endif

#ifdef CONFIG_ABI_TRACE
#include <abi/trace.h>
#endif


int abi_brk(unsigned long newbrk)
{
	if (!newbrk)
		return current->mm->brk;
	if (newbrk != current->mm->brk && (unsigned long)SYS(brk)(newbrk) != newbrk)
		return -ENOMEM;
	return 0;
}

EXPORT_SYMBOL(abi_brk);

#ifdef __sparc__
int abi_fork(struct pt_regs * regs) {
       /* No fork yet */
       printk ("ibcs2/sparc: No fork yet\n");
       send_sig(SIGSEGV, current, 1);
       return -1;
}

EXPORT_SYMBOL(abi_fork);

int abi_wait(struct pt_regs * regs) {
       /* No fork yet */
       printk ("ibcs2/sparc: No wait yet\n");
       send_sig(SIGSEGV, current, 1);
       return -1;
}

EXPORT_SYMBOL(abi_wait);

int abi_exec(struct pt_regs * regs)
{

       /* No exec yet */
       printk ("ibcs2/sparc: No fork yet\n");
       send_sig(SIGSEGV, current, 1);
       return -1;
}

EXPORT_SYMBOL(abi_exec);

int abi_pipe(struct pt_regs * regs)
{
       long filedes[2];
       mm_segment_t old_fs = get_fs();
       int rvalue;

       set_fs(get_ds());
       rvalue = SYS(pipe)(&filedes);
       set_fs(old_fs);
       if (rvalue == 0) {
               rvalue = filedes[0];
               regs->u_regs [UREG_I0] = filedes[1];
       }
       return rvalue;
}

EXPORT_SYMBOL(abi_pipe);

int abi_getpid(struct pt_regs * regs)
{
       return current->pid;
}

EXPORT_SYMBOL(abi_getpid);

int abi_getuid(struct pt_regs * regs)
{
       return current->uid;
}

EXPORT_SYMBOL(abi_getuid);

int abi_getgid(struct pt_regs * regs)
{
       return current->gid;
}

EXPORT_SYMBOL(abi_getgid);

#else /* __sparc__ */

int abi_lseek(int fd, unsigned long offset, int whence)
{
	int error;
	struct file *file;
	struct inode *inode;

	error = SYS(lseek)(fd, offset, whence);
	if (error != -ESPIPE || !personality(PER_SCOSVR3))
		return error;

	file = fget(fd);
	if (!file)
		goto out;
	inode = file->f_dentry->d_inode;
	if (inode && (S_ISCHR(inode->i_mode) || S_ISBLK(inode->i_mode)))
		error = 0;
	fput(file);
out:
	return error;
}

EXPORT_SYMBOL(abi_lseek);

int abi_fork(struct pt_regs * regs)
{
	int rvalue;

	regs->eflags &= ~1; /* Clear carry flag */
	rvalue = SYS(fork)(regs->ebx, regs->ecx, 1,
		regs->esi, regs->edi, regs->ebp, regs->eax, regs->xds,
		regs->xes, regs->orig_eax,
		regs->eip, regs->xcs, regs->eflags, regs->esp, regs->xss);
	regs->edx = 0;
	return rvalue;
}

EXPORT_SYMBOL(abi_fork);

int abi_pipe(struct pt_regs * regs)
{
	long filedes[2];
	mm_segment_t old_fs = get_fs();
	int rvalue;

	set_fs(get_ds());
	rvalue = SYS(pipe)(&filedes);
	set_fs(old_fs);
	if (rvalue == 0) {
		rvalue = filedes[0];
		regs->edx = filedes[1];
	}
	return rvalue;
}

EXPORT_SYMBOL(abi_pipe);

/* note the double value return in eax and edx */
int abi_getpid(struct pt_regs * regs)
{
	regs->edx = current->p_pptr->pid;

	return current->pid;
}

EXPORT_SYMBOL(abi_getpid);

/* note the double value return in eax and edx */
int abi_getuid(struct pt_regs * regs)
{
	regs->edx = current->euid;

	return current->uid;
}

EXPORT_SYMBOL(abi_getuid);

/* note the double value return in eax and edx */
int abi_getgid(struct pt_regs * regs)
{
	regs->edx = current->egid;

	return current->gid;
}

EXPORT_SYMBOL(abi_getgid);

#define FLAG_ZF 0x0040
#define FLAG_PF 0x0004
#define FLAG_SF 0x0080
#define FLAG_OF 0x0800

#define MAGIC_WAITPID_FLAG (FLAG_ZF | FLAG_PF | FLAG_SF | FLAG_OF)

int abi_wait(struct pt_regs * regs)
{
	long	result, kopt;
	int	pid, loc, opt;
	mm_segment_t	old_fs;

	/* xenix wait() puts status to edx and returns pid */
	if (personality(PER_XENIX)) {
		old_fs = get_fs();
		set_fs (get_ds());
		result = SYS(wait4)(-1, &loc, 0, NULL);
		set_fs(old_fs);

		regs->edx = loc;
		return result;
	}
	/* if ZF,PF,SF,and OF are set then it is waitpid */
	if ((regs->eflags & MAGIC_WAITPID_FLAG) == MAGIC_WAITPID_FLAG) {
		get_user(pid, ((unsigned long *) regs->esp) + 1);
		get_user(loc, ((unsigned long *) regs->esp) + 2);
		get_user(opt, ((unsigned long *) regs->esp) + 3);

		/* Now translate the options from the SVr4 numbers */
		kopt = 0;
		if (opt & 0100) kopt |= WNOHANG;
		if (opt & 4) kopt |= WUNTRACED;

		result = SYS(wait4)(pid, loc, kopt, NULL);
	} else {
		get_user(loc, ((unsigned long *) regs->esp) + 1);
		result = SYS(wait4)(-1, loc, WUNTRACED, NULL);
	}
	if (result >= 0 && loc) {
		get_user(regs->edx, (unsigned long *) loc);
		if ((regs->edx & 0xff) == 0x7f) {
			int sig = (regs->edx >> 8) & 0xff;
			if (sig < NSIGNALS)
				sig = current->exec_domain->signal_map[sig];
			regs->edx = (regs->edx & (~0xff00)) | (sig << 8);
			put_user(regs->edx, (unsigned long *)loc);
		} else if (regs->edx && regs->edx == (regs->edx & 0xff)) {
			if ((regs->edx & 0x7f) < NSIGNALS)
				regs->edx = current->exec_domain->signal_map[regs->edx & 0x7f];
			put_user(regs->edx, (unsigned long *)loc);
		}
	}
	return result;
}

EXPORT_SYMBOL(abi_wait);

/*
 * abi_exec() executes a new program.
 */
int abi_exec(struct pt_regs *regs)
{
	int error;
	char *pgm, **argv, **envp;
	char *filename;

	get_user(pgm, ((unsigned long *) regs->esp) + 1);
	get_user(argv, ((unsigned long *) regs->esp) + 2);
	get_user(envp, ((unsigned long *) regs->esp) + 3);
#ifdef CONFIG_ABI_TRACE
	if ((ibcs_trace & TRACE_API) || ibcs_func_p->trace) {
		int i;
		char **v, *p, *q;

		q = getname(pgm);
		if (IS_ERR(q)) {
			printk(KERN_DEBUG "iBCS:       pgm: 0x%lx pointer error %ld\n",
				(unsigned long)pgm, PTR_ERR(q));
		} else {
			printk(KERN_DEBUG "iBCS        pgm: 0x%lx \"%s\"\n",
				(unsigned long)pgm, q);
			putname(q);
		}

		for (i=0,v=argv; v && i < 20; v++,i++) {
			if (get_user(p, v) || !p)
				break;
			q = getname(p);
			if (IS_ERR(q)) {
				printk(KERN_DEBUG "iBCS        arg: 0x%lx pointer error %ld\n",
					(unsigned long)p, PTR_ERR(q));
			} else {
				printk(KERN_DEBUG "iBCS:       arg: 0x%lx \"%s\"\n",
					(unsigned long)p, q);
				putname(q);
			}
		}
		if (v && p)
			printk(KERN_DEBUG "iBCS:       arg: ...\n");

		for (i=0,v=envp; v && i < 20; v++,i++) {
			if (get_user(p, v) || !p)
				break;
			q = getname(p);
			if (IS_ERR(q)) {
				printk(KERN_DEBUG "iBCS        env: 0x%lx pointer error %ld\n",
					(unsigned long)p, PTR_ERR(q));
			} else {
				printk(KERN_DEBUG "iBCS:       env: 0x%lx \"%s\"\n",
					(unsigned long)p, q);
				putname(q);
			}
		}
		if (v && p)
			printk(KERN_DEBUG "iBCS:       env: ...\n");
	}
#endif

	filename = getname(pgm);
	error = PTR_ERR(filename);
	if (!IS_ERR(filename)) {
		/* if you get an error on this undefined, then remove the */
		/* 'static' declaration in /linux/fs/exec.c */
		error = do_execve(filename, argv, envp, regs);
		putname (filename);
        }
	return error;
}

EXPORT_SYMBOL(abi_exec);

int abi_procids(struct pt_regs * regs)
{
	int op, arg_offset;

	get_user(op, ((unsigned long *)regs->esp)+1);

	/* Remap op codes for current personality if necessary. */
	switch (current->personality & PER_MASK) {
		case (PER_SVR3 & PER_MASK):
		case (PER_SCOSVR3 & PER_MASK):
		case (PER_WYSEV386 & PER_MASK):
		case (PER_XENIX & PER_MASK): {
			if (op < 0 || op > 5)
				return -EINVAL;
			op = "\000\001\005\003\377\377"[op];

			/* SCO at least uses an interesting library to
			 * syscall mapping that leaves an extra return
			 * address between the op code and the arguments.
			 */
			arg_offset = 1;
			break;
		}

		default:
			arg_offset = 0;
	}

	switch (op) {
		case 0: /* getpgrp */
			return current->pgrp;

		case 1: /* setpgrp */
			SYS(setpgid)(0, 0);
 			current->tty=NULL;
			return current->pgrp;

		case 2: { /* getsid */
			unsigned long pid;
			get_user(pid, ((unsigned long *)regs->esp)
						+ 2 + arg_offset);
			return SYS(getsid)(pid);
		}

		case 3: /* setsid */
			return SYS(setsid)();

		case 4: { /* getpgid */
			unsigned long pid;
			get_user(pid, ((unsigned long *)regs->esp)
						+ 2 + arg_offset);
			return SYS(getpgid)(pid);
		}

		case 5: { /* setpgid */
			int pid, pgid;

			get_user(pid, ((unsigned long *)regs->esp)
						+ 2 + arg_offset);
			get_user(pgid, ((unsigned long *)regs->esp)
						+ 3 + arg_offset);
			return SYS(setpgid)(pid, pgid);
		}
	}

	return -EINVAL;
}
EXPORT_SYMBOL(abi_procids);

#endif /* __sparc__ */

int abi_read(int fd, char *buf, int nbytes)
{
	int error, here, posn, reclen;
	struct file *file;
	struct dirent *d;
	mm_segment_t old_fs;

	error = SYS(read)(fd, buf, nbytes);
	if (error != -EISDIR)
		return error;

	/* Stupid bloody thing is trying to read a directory. Some old
	 * programs expect this to work. It works on SCO. To emulate it
	 * we have to map a dirent to a direct. This involves shrinking
	 * a long inode to a short. Fortunately nothing this archaic is
	 * likely to care about anything but the filenames of entries
	 * with non-zero inodes.
	 */

	file = fget(fd);
	if (!file)
		return -EBADF;

	d = (struct dirent *)get_free_page(GFP_KERNEL);
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
		reclen = 16 * ((d->d_reclen + 13) / 14);
		if (posn + reclen <= nbytes) {
			/* SCO (at least) handles long filenames by breaking
			 * them up in to 14 character chunks of which all
			 * but the last have the inode set to 0xffff.
			 */
			char *p = d->d_name;

			/* Put all but the last chunk. */
			while (d->d_reclen > 14) {
				put_user(0xffff, (unsigned short *)(buf+posn));
				posn += 2;
				copy_to_user(buf+posn, p, 14);
				posn += 14;
				p += 14;
				d->d_reclen -= 14;
			}
			/* Put the last chunk. Note the we have to fold a
			 * long inode number down to a short avoiding
			 * giving a zero inode number since that indicates
			 * an unused directory slot. Note also that the
			 * folding used here must match that used in stat()
			 * or path finding programs that do read() on
			 * directories will fail.
			 */
#if 0
			/* This appears to match what SCO does for
			 * reads on a directory with long inodes.
			 */
			if ((unsigned long)d->d_ino > 0xfffe)
				put_user(0xfffe, (unsigned short *)(buf+posn));
			else
				put_user((short)d->d_ino, (unsigned short *)(buf+posn));
#else
			/* This attempts to match the way stat and
			 * getdents fold long inodes to shorts.
			 */
			if ((unsigned long)d->d_ino & 0xffff)
				put_user((unsigned long)d->d_ino & 0xffff,
					(unsigned short *)(buf+posn));
			else
				put_user(0xfffe, (unsigned short *)(buf+posn));
#endif
			posn += 2;
			copy_to_user(buf+posn, p, d->d_reclen);

			/* Ensure that filenames that don't fill the array
			 * completely are null filled.
			 */
			for (; d->d_reclen < 14; d->d_reclen++)
				put_user('\0', buf+posn+d->d_reclen);

			posn += 14;
		} else if (posn) {
			SYS(lseek)(fd, here, 0);
		} /* else posn == 0 */
	}

	free_page((unsigned long)d);
	fput(file);

	/* If we've put something in the buffer return the byte count
	 * otherwise return the error status.
	 */
	return (posn ? posn : error);
}

EXPORT_SYMBOL(abi_read);

#ifdef CONFIG_ABI_TRACE
int abi_select(int n, void *rfds, void *wfds, void *efds, struct timeval *t)
{
	if ((ibcs_trace & TRACE_API) || ibcs_func_p->trace) {
		if (t) {
			struct timeval tv;
			int error;

			error = get_user(tv.tv_sec, &(t->tv_sec));
			if (!error)
				error = get_user(tv.tv_usec, &(t->tv_usec));
			if (error)
				return error;
			printk(KERN_DEBUG "iBCS: select timeout in %lus, %luus\n",
				tv.tv_sec, tv.tv_usec);
		}
	}
	return SYS(_newselect)(n, rfds, wfds, efds, t);
}

EXPORT_SYMBOL(abi_select);

#endif


int
abi_time(void)
{
	return SYS(time)(0);
}

EXPORT_SYMBOL(abi_time);


int abi_mknod(const char *fname, int mode, int dev)
{
	/* Linux doesn't allow us to create a directory with mknod(). */
	if ((mode & 0017000) == 0040000)
		return abi_mkdir(fname, mode);
	return SYS(mknod)(fname, mode, dev);
}

EXPORT_SYMBOL(abi_mknod);

/*
 *  Translate the signal number to the corresponding item for Linux.
 */
static inline int abi_mapsig(int sig)
{
	if ((unsigned int) sig >= NSIGNALS)
		return -1;
	return current->exec_domain->signal_map[sig];
}

int abi_kill(int pid, int sig) {
	int outsig = abi_mapsig(sig & 0xFF);

#ifdef CONFIG_ABI_TRACE
	if ((ibcs_trace & TRACE_SIGNAL) || ibcs_func_p->trace)
		printk (KERN_DEBUG "ibcs_kill:	insig (%d)	outsig(%d) \n"
			, sig & 0xFF, outsig);
#endif
	if (outsig < 0) {
		return -EINVAL;
	}
	return SYS(kill) (pid, outsig);
}

EXPORT_SYMBOL(abi_kill);

int abi_mkdir(const char *fname, int mode)
{
	int error;
	mm_segment_t old_fs;
	char *tmp, *p;

	tmp = getname(fname);
	if (IS_ERR(tmp))
		return PTR_ERR(tmp);

	/* Drop any trailing slash */
	for (p=tmp; *p; p++);
	p--;
	if (*p == '/')
		*p = '\0';

	old_fs = get_fs();
	set_fs(get_ds());
	error = SYS(mkdir)(tmp, mode);
	set_fs(old_fs);

	putname(tmp);
	return error;
}

EXPORT_SYMBOL(abi_mkdir);
