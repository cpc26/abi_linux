/*
 *  Function prototypes used for SVR4 emulation.
 *
 * $Id$
 * $Source$
 */
#include <linux/ptrace.h>	/* for pt_regs */
#include <linux/sched.h>
#include <linux/signal.h>
#include <linux/unistd.h>

#define SVR_NCC 8
struct svr_termio {
	unsigned short c_iflag;
	unsigned short c_oflag;
	unsigned short c_cflag;
	unsigned short c_lflag;
	char c_line;
	unsigned char c_cc[SVR_NCC];
};

#define SVR4_NCCS (19)
struct svr4_termios {
	unsigned long c_iflag;
	unsigned long c_oflag;
	unsigned long c_cflag;
	unsigned long c_lflag;
	unsigned char c_cc[SVR4_NCCS];
};

/* timod.c */
extern int svr4_ioctl_sockmod(int fd, unsigned int func, void *arg);

/* svr4.c */
extern int svr4_getgroups(int n, unsigned long * buf);
extern int svr4_setgroups(int n, unsigned long * buf);
extern int svr4_waitsys(struct pt_regs * regs);
extern int svr4_seteuid(int uid);
extern int svr4_setegid(int gid);

/* open.c */
extern int svr4_open(const char * fname, int flag, int mode);
extern int svr4_statfs(const char * path, struct ibcs_statfs * buf, int len, int fstype);
extern int svr4_fstatfs(unsigned int fd, struct ibcs_statfs * buf, int len, int fstype);
extern int svr4_getdents(int fd, char * buf, int nybtes);

/* ipc.c */
extern int svr4_semsys(struct pt_regs * regs);
extern int svr4_shmsys(struct pt_regs * regs);
extern int svr4_msgsys(struct pt_regs * regs);

/* stream.c */
extern int svr4_getmsg(struct pt_regs *regs);
extern int svr4_putmsg(struct pt_regs *regs);
extern int svr4_getpmsg(struct pt_regs *regs);
extern int svr4_putpmsg(struct pt_regs *regs);

/* ioctl.c */
extern int svr4_ioctl_stream(struct pt_regs *regs, int fd, unsigned int func, void *arg);
extern int svr4_ioctl_termiox(int fd, unsigned int func, void *arg);

/* sysfs.c */
extern int svr4_sysfs(int cmd, int arg1, int arg2);

/* sysinfo.c */
extern int svr4_sysinfo(int, char *, long);

/* xstat.c */
extern int svr4_fxstat(int vers, int fd, void * buf);

/* ulimit.c */
extern int svr4_ulimit(int cmd, int val);
extern int svr4_getrlimit(int cmd, void *val);
extern int svr4_setrlimit(int cmd, void *val);

/* poll.c */
struct poll{
	int fd;
	short events;
	short revents;
};
extern int svr4_poll(struct poll * ufds, size_t nfds, int timeout);

/* hrtsys.c */
extern int svr4_hrtsys(struct pt_regs * regs);

/* syslocal.c */
extern int svr4_syslocal(struct pt_regs * regs);

/* ptrace.c */
extern int svr4_ptrace(int req, int pid, unsigned long addr, unsigned long data);

/* fcntl.c */
extern int svr4_fcntl(struct pt_regs *regs);

/* mmap.c */
extern int svr4_mmap(unsigned int vaddr, unsigned int vsize, int prot,
 		     int flags, int fd, unsigned int file_offset);

/* sysconf.c */
#define _CONFIG_NGROUPS          2       /* # configured supplemental groups */
#define _CONFIG_CHILD_MAX        3       /* max # of processes per uid session */
#define _CONFIG_OPEN_FILES       4       /* max # of open files per process */
#define _CONFIG_POSIX_VER        5       /* POSIX version */
#define _CONFIG_PAGESIZE         6       /* system page size */
#define _CONFIG_CLK_TCK          7       /* ticks per second */
#define _CONFIG_XOPEN_VER        8       /* XOPEN version */
#define _CONFIG_NACLS_MAX        9       /* for Enhanced Security */
#define _CONFIG_ARG_MAX          10      /* max length of exec args */
#define _CONFIG_NPROC            11      /* # processes system is config for */
#define _CONFIG_NENGINE          12      /* # configured processors (CPUs) */
#define _CONFIG_NENGINE_ONLN     13      /* # online processors (CPUs) */
#define _CONFIG_TOTAL_MEMORY     14      /* total memory */
#define _CONFIG_USEABLE_MEMORY   15      /* user + system memory */
#define _CONFIG_GENERAL_MEMORY   16      /* user only memory */
#define _CONFIG_DEDICATED_MEMORY 17     /* dedicated memory */
#define _CONFIG_NCGS_CONF        18      /* # CGs in system */
#define _CONFIG_NCGS_ONLN        19      /* # CGs online now */
#define _CONFIG_MAX_ENG_PER_CG   20      /* max engines per CG */
#define _CONFIG_CACHE_LINE       21      /* memory cache line size */
#define _CONFIG_SYSTEM_ID        22      /* system id assigned at ISL */
#define _CONFIG_KERNEL_VM        23      /* size of kernel virtual memory */
extern int svr4_sysconfig(int name);

/* sysi86.c */
extern int svr4_sysi86(struct pt_regs * regs);

/* svr4_funcs.c */
extern struct map_segment svr4_err_map[];
extern struct map_segment svr4_socktype_map[];
extern struct map_segment abi_sockopt_map[];
extern struct map_segment abi_af_map[];
extern long linux_to_ibcs_signals[];
extern long ibcs_to_linux_signals[];
