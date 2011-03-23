/*
 *  Function prototypes used by the iBCS2 emulator
 *
 * $Id$
 * $Source$
 */

#ifndef __IBCS_IBCS_H__
#define __IBCS_IBCS_H__
#include <linux/ptrace.h>	/* for pt_regs */
#include <linux/sched.h>
#include <linux/signal.h>
#include <linux/unistd.h>
#include <linux/config.h>

#include <abi/stream.h>
#include <abi/map.h>
#include <asm/abi.h>

/* This is straight from linux/fs/stat.c. It is required for
 * proper NFS attribute caching (so it says there). Maybe the
 * kernel should export it - but it is basically simple...
 */
static __inline__ int
do_revalidate(struct dentry *dentry)
{
	struct inode *inode = dentry->d_inode;
	if (inode->i_op && inode->i_op->revalidate)
		return inode->i_op->revalidate(dentry);
	return 0;
}


static inline unsigned short map_flags(unsigned short f, unsigned short map[])
{
        int i;
        unsigned short m, r;

        r = 0;
        for (i=0,m=1; i < 16; i++,m<<=1)
                if (f & m)
                        r |= map[i];

        return r;
}


/* XXX anyone has an idea how to make gcc shut up about this? */
typedef int (*sysfun_p)();

extern sysfun_p sys_call_table[];

#define SYS(name)	(sys_call_table[__NR_##name])


/*
 * the function prefix sys_... are used by linux in native mode.
 * abi_, svr4_, sol_, uw7_ and ibcs_... are emulation interfaces for routines that 
 * differ from iBCS2 and linux.  The xnx_... are xenix routines.
 */
typedef struct abi_function {
	void *	kfunc;	/* function to call (sys_..., ibcs_... or xnx_...)
			 * or pointer to a sub class.
			 */
	short	nargs;	/* number of args to kfunc or Ukn, Spl or Fast */
#ifdef CONFIG_ABI_TRACE
	short	trace;	/* trace function we can turn tracing on or off */
	char *	name;	/* name of function (for tracing) */
	char *	args;	/* how to print the arg list (see plist) */
#endif
} ABI_func;

struct ibcs_statfs {
	short f_type;
	long f_bsize;
	long f_frsize;
	long f_blocks;
	long f_bfree;
	long f_files;
	long f_ffree;
	char f_fname[6];
	char f_fpack[6];
};


#ifdef __sparc__

typedef struct {
	long tv_sec;
	long tv_nsec;
} timestruct_t;

struct ibcs_stat {
	unsigned long st_dev;
	long          st_pad1[3];     /* network id */
	unsigned long st_ino;
	unsigned long st_mode;
        unsigned long st_nlink;
        unsigned long st_uid;
        unsigned long st_gid;
        unsigned long st_rdev;
        long          st_pad2[2];
        long          st_size;
        long          st_pad3;        /* st_size, off_t expansion */
        timestruct_t  st_atime;
        timestruct_t  st_mtime;
        timestruct_t  st_ctime;
        long          st_blksize;
        long          st_blocks;
        char          st_fstype[16];
        long          st_pad4[8];     /* expansion area */
};
#else
struct ibcs_stat {
        unsigned short st_dev;
        unsigned short st_ino;
        unsigned short st_mode;
        unsigned short st_nlink;
        unsigned short st_uid;
        unsigned short st_gid;
        unsigned short st_rdev;
        unsigned long  st_size;
        unsigned long  st_atime;
        unsigned long  st_mtime;
        unsigned long  st_ctime;
};
#endif


struct ibcs_iovec {
	unsigned long addr;
	int len;
};

/* coff.c */
extern int abi_brk(unsigned long newbrk);
extern int abi_lseek(int fd, unsigned long offset, int whence);
extern int abi_fork(struct pt_regs * regs);
extern int abi_pipe(struct pt_regs * regs);
extern int abi_getpid(struct pt_regs * regs);
extern int abi_getuid(struct pt_regs * regs);
extern int abi_getgid(struct pt_regs * regs);
extern int abi_wait(struct pt_regs * regs);
extern int ibcs_execv(struct pt_regs * regs);
extern int abi_exec(struct pt_regs * regs);
extern int abi_read(int fd, char *buf, int nbytes);
extern int abi_procids(struct pt_regs * regs);
extern int abi_select(int n, void *rfds, void *wfds, void *efds,
			struct timeval *t);
extern int abi_time(void);
extern int ibcs_writev(int fd, struct ibcs_iovec *it, int n);

extern int abi_syscall(struct pt_regs *regs);

/* open.c */
extern int abi_mkdir(const char *fname, int mode);
extern int abi_mknod(const char *fname, int mode, int dev);

/* secureware.c */
extern int sw_security(int cmd, void *p1, void *p2, void *p3, void *p4, void *p5);

/* signal.c */
/* For mapping signal numbers */
void deactivate_signal(struct task_struct *task, int signum);
extern int abi_sigfunc(struct pt_regs * regs);
extern int abi_kill(int pid, int sig);

/* stat.c */
extern int abi_stat(char * filename, struct ibcs_stat * statbuf);
extern int abi_lstat(char * filename, struct ibcs_stat * statbuf);
extern int abi_fstat(unsigned int fd, struct ibcs_stat * statbuf);

/* svr4.c */
struct svr4_siginfo {
	int si_signo;
	int si_code;
	int si_errno;
	union {
		struct {	/* kill(), SIGCLD */
			long _pid;
			union {
				struct {
					long _uid;
				} _kill;
				struct {
					long _utime;
					int _status;
					long _stime;
				} _cld;
			} _pdata;
		} _proc;
		struct {	/* SIGSEGV, SIGBUS, SIGILL, SIGFPE */
			char *_addr;
		} _fault;
		struct {	/* SIGPOLL, SIGXFSZ */
			int _fd;
			long _band;
		} _file;
	} _data;
};
#define SVR4_CLD_EXITED		1
#define SVR4_CLD_KILLED		2
#define SVR4_CLD_DUMPED		3
#define SVR4_CLD_TRAPPED	4
#define SVR4_CLD_STOPPED	5
#define SVR4_CLD_CONTINUED	6
extern int svr4_getgroups(int n, unsigned long *buf);
extern int svr4_setgroups(int n, unsigned long *buf);
extern int svr4_waitid(int idtype, int id, struct svr4_siginfo *infop, int options);
extern int svr4_access(char *path, int mode);

/* sysconf.c */
extern int ibcs_sysconf(int name);

/* utsname.c */
extern int abi_utsname(unsigned long addr);
extern int sco_utsname(unsigned long addr);
extern int v7_utsname(unsigned long addr);

/* socket.c */
extern int abi_do_setsockopt(unsigned long *sp);
extern int abi_do_getsockopt(unsigned long *sp);

/* proc.c */
extern int abi_proc_init(void);
extern void abi_proc_cleanup(void);

/* wysev386.c */
extern int abi_gethostname(char *name, int len);
extern int abi_getdomainname(char *name, int len);
extern int abi_wait3(int *loc);
extern int abi_socket(struct pt_regs *regs);
extern int abi_connect(struct pt_regs *regs);
extern int abi_accept(struct pt_regs *regs);
extern int abi_send(struct pt_regs *regs);
extern int abi_recv(struct pt_regs *regs);
extern int abi_bind(struct pt_regs *regs);
extern int abi_setsockopt(struct pt_regs *regs);
extern int abi_listen(struct pt_regs *regs);
extern int abi_getsockopt(struct pt_regs *regs);
extern int abi_recvfrom(struct pt_regs *regs);
extern int abi_sendto(struct pt_regs *regs);
extern int abi_shutdown(struct pt_regs *regs);
extern int abi_socketpair(struct pt_regs *regs);
extern int abi_getpeername(struct pt_regs *regs);
extern int abi_getsockname(struct pt_regs *regs);

/* ioctl.c */
extern int bsd_ioctl_termios(int fd, unsigned int func, void *arg);

/* signal.c */
extern int abi_sigsuspend(struct pt_regs *regs);

/* From wysev386i.c */
extern int wv386_ioctl(int fd, unsigned int ioctl_num, void *arg);

/* From socksys.c */
extern struct proc_dir_entry * abi_proc_entry;
extern int socksys_major;
extern void inherit_socksys_funcs(unsigned int fd, int state);
extern int abi_socksys_fd_init(int fd, int rw, const char *buf, int *count);
extern int socksys_syscall(int *sp);
extern int abi_ioctl_socksys(int fd, unsigned int cmd, void *arg);


/* From sysisc.c */
extern int isc_setostype(int);

/* From vtkd.c */
extern int ibcs_ioctl_vtkd(int, int, void *);



#define SC(name)	(void *)__NR_##name

#ifdef CONFIG_ABI_TRACE
#  define ITR(trace, name, args)	,trace,name,args
#else
#  define ITR(trace, name, args)
#endif


/* This table contains the appropriate kernel routines that can be run
 * to perform the syscalls in question.  If an entry is 'Ukn' we don't
 * know how to handle it yet. (We also set trace on by default for these)
 * Spl means that we need to do special processing for this syscall
 *	(see ibcs_wait or ibcs_getpid)
 * Fast means that even the error return handling is done by the function call.
 */
#define ZERO	0x64	/* Um, magic zero for callmap. Don't ask :-). */
#define Spl	0x65	/* pass the regs structure down */
#define Ukn	0x66	/* no code to handle this case yet */
#define Fast	0x67	/* magic on return, return regs structure already set up */

void abi_dispatch(struct pt_regs *regs, ABI_func *p, int offset);


/* Translate the errno numbers from linux to current personality.
 * This should be removed and all other sources changed to call the
 * map function above directly.
 */
static inline int iABI_errors(int lnx_errno)
{
	return map_value(current->exec_domain->err_map, lnx_errno, 1);
}


#endif /* __IBCS_IBCS_H__ */
