#include <linux/config.h>
#include <linux/module.h>
#include <abi/abi.h>
#include <abi/svr4.h>
#include <abi/abi4.h>
#include <abi/map.h>
#include <abi/signal.h>


long linux_to_ibcs_signals[NSIGNALS+1] = {
	0,
	IBCS_SIGHUP,	IBCS_SIGINT,	IBCS_SIGQUIT,	IBCS_SIGILL,
	IBCS_SIGTRAP,	IBCS_SIGABRT,	-1,		IBCS_SIGFPE,
	IBCS_SIGKILL,	IBCS_SIGUSR1,	IBCS_SIGSEGV,	IBCS_SIGUSR2,
	IBCS_SIGPIPE,	IBCS_SIGALRM,	IBCS_SIGTERM,	IBCS_SIGSEGV,
	IBCS_SIGCHLD,	IBCS_SIGCONT,	IBCS_SIGSTOP,	IBCS_SIGTSTP,
	IBCS_SIGTTIN,	IBCS_SIGTTOU,	IBCS_SIGURG,	IBCS_SIGGXCPU,
	IBCS_SIGGXFSZ,	IBCS_SIGVTALRM,	IBCS_SIGPROF,	IBCS_SIGWINCH,
	IBCS_SIGIO,	IBCS_SIGPWR,	-1,		-1
};

long ibcs_to_linux_signals[NSIGNALS+1] = {
	0,
	SIGHUP,		SIGINT,		SIGQUIT,	SIGILL,
	SIGTRAP,	SIGIOT,		SIGUNUSED,	SIGFPE,
	SIGKILL,	SIGUNUSED,	SIGSEGV,	SIGUNUSED,
	SIGPIPE,	SIGALRM,	SIGTERM,	SIGUSR1,
	SIGUSR2,	SIGCHLD,	SIGPWR,		SIGWINCH,
	SIGURG,		SIGPOLL,	SIGSTOP,	SIGTSTP,
	SIGCONT,	SIGTTIN,	SIGTTOU,	SIGVTALRM,
	SIGPROF,	SIGXCPU,	SIGXFSZ,	-1
};

EXPORT_SYMBOL(linux_to_ibcs_signals);
EXPORT_SYMBOL(ibcs_to_linux_signals);

extern void iBCS_class_XNX(struct pt_regs *regs);
extern void iBCS_class_ISC(struct pt_regs *regs);

static char type_svr4_to_linux_seg1[] = {
        SOCK_DGRAM,
        SOCK_STREAM,
        0,
        SOCK_RAW,
        SOCK_RDM,
        SOCK_SEQPACKET
};

struct map_segment svr4_socktype_map[] = {
        /* 1 to 6 are remapped as indicated. Nothing else is valid. */
        { 1, 6, type_svr4_to_linux_seg1 },
        { -1 }
};

EXPORT_SYMBOL(svr4_socktype_map);

/* Map Linux RESTART* values (512,513,514) to EINTR */
static unsigned char LNX_err_table[] = {
        EINTR, EINTR, EINTR
};


/* Default Linux to iBCS mapping.
 * We could remove some of the long identity mapped runs but at the
 * expense of extra comparisons for each mapping at run time...
 */
static unsigned char SVR4_err_table[] = {
          0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
         16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
         32, 33, 34, 45, 78, 46, 89, 93, 90, 90, 35, 36, 37, 38, 39, 40,
         41, 42, 43, 44, 50, 51, 52, 53, 54, 55, 56, 57, 60, 61, 62, 63,
         64, 65, 66, 67, 68, 69, 70, 71, 74, 76, 77, 79, 80, 81, 82, 83,
         84, 85, 86, 87, 88, 91, 92, 94, 95, 96, 97, 98, 99,120,121,122,
        123,124,125,126,127,128,129,130,131,132,133,134,143,144,145,146,
        147,148,149,150, 22,135,137,138,139,140, 28
};


/* SVR4 (aka the full official iBCS) is the base mapping - no exceptions,
 * other than the RESTART* values.
 */
struct map_segment svr4_err_map[] = {
        { 0,    0+sizeof(SVR4_err_table)-1,     SVR4_err_table },
        { 512,  512+sizeof(LNX_err_table)-1,    LNX_err_table },
        { -1 }
};

EXPORT_SYMBOL(svr4_err_map);

ABI_func svr4_generic_funcs[] = {
   { abi_syscall,	Fast	ITR(0, "syscall",	"")	}, /*    0 */
   { SC(exit),		-1	ITR(0, "exit",		"d")	}, /*    1 */
   { abi_fork,		Spl	ITR(0, "fork",		"")	}, /*    2 */
   { abi_read,		3	ITR(0, "read",		"dpd")	}, /*    3 */
   { SC(write),		-3	ITR(0, "write",		"dpd")	}, /*    4 */
   { svr4_open,		3	ITR(0, "open",		"soo")	}, /*    5 */
   { SC(close),		-1	ITR(0, "close",		"d")	}, /*    6 */
   { abi_wait,		Spl	ITR(0, "wait",		"xxx")	}, /*    7 */
   { SC(creat),		-2	ITR(0, "creat",		"so")	}, /*    8 */
   { SC(link),		-2	ITR(0, "link",		"ss")	}, /*    9 */
   { SC(unlink),	-1	ITR(0, "unlink",	"s")	}, /*   10 */
   { abi_exec,		Spl	ITR(0, "exec",		"sxx")	}, /*   11 */
   { SC(chdir),		-1	ITR(0, "chdir",		"s")	}, /*   12 */
   { abi_time,		0	ITR(0, "time",		"")	}, /*   13 */
   { abi_mknod,	3	ITR(0, "mknod",		"soo")	}, /*   14 */
   { SC(chmod),		-2	ITR(0, "chmod",		"so")	}, /*   15 */
   { SC(chown),		-3	ITR(0, "chown",		"sdd")	}, /*   16 */
   { abi_brk,		1	ITR(0, "brk/break",	"x")	}, /*   17 */
   { abi_stat,		2	ITR(0, "stat",		"sp")	}, /*   18 */
   { abi_lseek,	3	ITR(0, "seek/lseek",	"ddd")	}, /*   19 */
   { abi_getpid,	Spl	ITR(0, "getpid",	"")	}, /*   20 */
   { 0,			Ukn	ITR(1, "mount",		"")	}, /*   21 */
   { SC(umount),	-1	ITR(0, "umount",	"s")	}, /*   22 */
   { SC(setuid),	-1	ITR(0, "setuid",	"d")	}, /*   23 */
   { abi_getuid,	Spl	ITR(0, "getuid",	"")	}, /*   24 */
   { SC(stime),		-1	ITR(0, "stime",		"d")	}, /*   25 */
   { svr4_ptrace,	4	ITR(0, "ptrace",	"xdxx") }, /*   26 */
   { SC(alarm),		-1	ITR(0, "alarm",		"d")	}, /*   27 */
   { abi_fstat,	2	ITR(0, "fstat",		"dp")	}, /*   28 */
   { SC(pause),		-ZERO	ITR(0, "pause",		"")	}, /*   29 */
   { SC(utime),		-2	ITR(0, "utime",		"xx")	}, /*   30 */
   { 0,			Ukn	ITR(0, "stty",		"")	}, /*   31 */
   { 0,			Ukn	ITR(1, "gtty",		"")	}, /*   32 */
   { SC(access),	-2	ITR(0, "access",	"so")	}, /*   33 */
   { SC(nice),		-1	ITR(0, "nice",		"d")	}, /*   34 */
   { svr4_statfs,	4	ITR(0, "statfs",	"spdd")	}, /*   35 */
   { SC(sync),		-ZERO	ITR(0, "sync",		"")	}, /*   36 */
   { abi_kill,		2	ITR(0, "kill",		"dd")	}, /*   37 */
   { svr4_fstatfs,	4	ITR(0, "fstatfs",	"dpdd")	}, /*   38 */
   { abi_procids,	Spl	ITR(0, "ibcs_procids",	"d")	}, /*   39 */
   { iBCS_class_XNX,	Fast	ITR(0, "cxenix",	"")	}, /*   40 */
   { SC(dup),		-1	ITR(0, "dup",		"d")	}, /*   41 */
   { abi_pipe,		Spl	ITR(0, "pipe",		"")	}, /*   42 */
   { SC(times),		-1	ITR(0, "times",		"p")	}, /*   43 */
   { SC(profil),	-4	ITR(0, "prof",		"xxxx")}, /*   44 */
   { 0,			Ukn	ITR(1, "lock/plock",	"")	}, /*   45 */
   { SC(setgid),	-1	ITR(0, "setgid",	"d")	}, /*   46 */
   { abi_getgid,	Spl	ITR(0, "getgid",	"")	}, /*   47 */
   { abi_sigfunc,	Fast	ITR(0, "sigfunc",	"xxx")	}, /*   48 */
   { svr4_msgsys,	Spl	ITR(0, "msgsys",	"dxddd")}, /*   49 */
   { svr4_sysi86,	Spl	ITR(0, "sysi86/sys3b",	"d")	}, /*   50 */
   { SC(acct),		-1	ITR(0, "acct/sysacct",	"x")	}, /*   51 */
   { svr4_shmsys,	Fast	ITR(0, "shmsys",	"ddxo")}, /*   52 */
   { svr4_semsys,	Spl	ITR(0, "semsys",	"dddx")}, /*   53 */
   { svr4_ioctl,	Spl	ITR(0, "ioctl",		"dxx")	}, /*   54 */
   { 0,			3	ITR(0, "uadmin",	"xxx")	}, /*   55 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /*   56 */
   { v7_utsname,	1	ITR(0, "utsys",		"x")	}, /*   57 */
   { SC(fsync),		-1	ITR(0, "fsync",		"d")	}, /*   58 */
   { abi_exec,		Spl	ITR(0, "execv",		"spp")	}, /*   59 */
   { SC(umask),		-1	ITR(0, "umask",		"o")	}, /*   60 */
   { SC(chroot),	-1	ITR(0, "chroot",	"s")	}, /*   61 */
   { svr4_fcntl,	Spl	ITR(0, "fcntl",		"dxx")	}, /*   62 */
   { svr4_ulimit,	2	ITR(0, "ulimit",	"xx")	}, /*   63 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /*   64 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /*   65 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /*   66 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /*   67 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /*   68 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /*   69 */
   { 0,			Ukn	ITR(1, "advfs",		"")	}, /*   70 */
   { 0,			Ukn	ITR(1, "unadvfs",	"")	}, /*   71 */
   { 0,			Ukn	ITR(1, "rmount",	"")	}, /*   72 */
   { 0,			Ukn	ITR(1, "rumount",	"")	}, /*   73 */
   { 0,			Ukn	ITR(1, "rfstart",	"")	}, /*   74 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /*   75 */
   { 0,			Ukn	ITR(1, "rdebug",	"")	}, /*   76 */
   { 0,			Ukn	ITR(1, "rfstop",	"")	}, /*   77 */
   { 0,			Ukn	ITR(1, "rfsys",		"")	}, /*   78 */
   { SC(rmdir),		-1	ITR(0, "rmdir",		"s")	}, /*   79 */
   { abi_mkdir,	2	ITR(0, "mkdir",		"so")	}, /*   80 */
   { svr4_getdents,	3	ITR(0, "getdents",	"dxd")	}, /*   81 */
   { 0,			Ukn	ITR(1, "libattach",	"")	}, /*   82 */
   { 0,			Ukn	ITR(1, "libdetach",	"")	}, /*   83 */
   { svr4_sysfs,	3	ITR(0, "sysfs",		"dxx")	}, /*   84 */
   { svr4_getmsg,	Spl	ITR(0, "getmsg",	"dxxx")	}, /*   85 */
   { svr4_putmsg,	Spl	ITR(0, "putmsg",	"dxxd")	}, /*   86 */
   { svr4_poll,		3	ITR(0, "poll",		"xdd")	}, /*   87 */
   { abi_lstat,	2	ITR(0, "lstat",		"sp")	}, /*   88 */
   { SC(symlink),	-2	ITR(0, "symlink",	"ss")	}, /*   89 */
   { SC(readlink),	-3	ITR(0, "readlink",	"spd")	}, /*   90 */
   { 0,			Ukn	ITR(0, "svr4_setgroups","dp")	}, /*   91 */
   { 0,			Ukn	ITR(0, "svr4_getgroups","dp")	}, /*   92 */
   { SC(fchmod),	-2	ITR(0, "fchmod",	"do")	}, /*   93 */
   { SC(fchown),	-3	ITR(0, "fchown",	"ddd")	}, /*   94 */
   { abi_sigprocmask,	3	ITR(0, "sigprocmask",	"dxx")	}, /*   95 */
   { abi_sigsuspend,	Spl	ITR(0, "sigsuspend",	"x")	}, /*   96 */
   { 0,			2	ITR(1, "sigaltstack",	"xx")	}, /*   97 */
   { abi_sigaction,	3	ITR(0, "sigaction",	"dxx")	}, /*   98 */
   { svr4_sigpending,	2	ITR(1, "sigpending",	"dp")	}, /*   99 */
   { svr4_context,	Spl	ITR(0, "context",	"")	}, /*   100 */
   { 0,			Ukn	ITR(1, "evsys",		"")	}, /*   101 */
   { 0,			Ukn	ITR(1, "evtrapret",	"")	}, /*   102 */
   { abi_statvfs,	2	ITR(0, "statvfs",	"sp")	}, /*   103 */
   { abi_statvfs,	2	ITR(0, "fstatvfs",	"dp")	}, /*   104 */
   { iBCS_class_ISC,	Fast	ITR(0, "sysisc",	"")	}, /*   105 */
   { 0,			Ukn	ITR(1, "nfssys",	"")	}, /*   106 */
   { 0,			4	ITR(0, "waitid",	"ddxd")	}, /*   107 */
   { 0,			3	ITR(1, "sigsendsys",	"ddd")	}, /*   108 */
   { svr4_hrtsys,	Spl	ITR(0, "hrtsys",	"xxx")	}, /*   109 */
   { 0,			3	ITR(1, "acancel",	"dxd")	}, /*   110 */
   { 0,			Ukn	ITR(1, "async",		"")	}, /*   111 */
   { 0,			Ukn	ITR(1, "priocntlsys",	"")	}, /*   112 */
   { svr4_pathconf,	2	ITR(1, "pathconf",	"sd")	}, /*   113 */
   { 0,			3	ITR(1, "mincore",	"xdx")	}, /*   114 */
   { svr4_mmap,		6	ITR(0, "mmap",		"xxxxdx") },/*   115 */
   { SC(mprotect),	-3	ITR(0, "mprotect",	"xdx")  },/*   116 */
   { SC(munmap),	-2	ITR(0, "munmap",	"xd")   },/*   117 */
   { svr4_fpathconf,	2	ITR(1, "fpathconf",	"dd")	}, /*   118 */
   { abi_fork,		Spl	ITR(0, "vfork",		"")	}, /*   119 */
   { SC(fchdir),	-1	ITR(0, "fchdir",	"d")	}, /*   120 */
   { SC(readv),		-3	ITR(0, "readv",		"dxd")	}, /*   121 */
   { SC(writev),	-3	ITR(0, "writev",	"dxd")	}, /*   122 */
   { svr4_xstat,	3	ITR(0, "xstat",		"dsx")	}, /*   123 */
   { svr4_lxstat,     	3	ITR(0, "lxstat",	"dsx")	}, /*   124 */
   { svr4_fxstat,	3	ITR(0, "fxstat",	"ddx")	}, /*   125 */
   { svr4_xmknod,	4	ITR(0, "xmknod",	"dsox")}, /*   126 */
   { svr4_syslocal,	Spl	ITR(0, "syslocal",	"d")	}, /*   127 */
   { svr4_getrlimit,	2	ITR(0, "setrlimit",	"dx")	}, /*   128 */
   { svr4_setrlimit,	2	ITR(0, "getrlimit",	"dx")	}, /*   129 */
   { 0,			3	ITR(1, "lchown",	"sdd")	}, /*   130 */
   { 0,			Ukn	ITR(1, "memcntl",	"")	}, /*   131 */
#ifdef CONFIG_ABI_XTI
   { svr4_getpmsg,	5	ITR(0, "getpmsg",	"dxxxx")}, /*   132 */
   { svr4_putpmsg,	5	ITR(0, "putpmsg",	"dxxdd")}, /*   133 */
#else
   { 0,			5	ITR(0, "getpmsg",	"dxxxx")}, /*   132 */
   { 0,			5	ITR(0, "putpmsg",	"dxxdd")}, /*   133 */
#endif
   { SC(rename),	-2	ITR(0, "rename",	"ss")	}, /*   134 */
   { abi_utsname,	1	ITR(0, "uname",		"x")	}, /*   135 */
   { svr4_setegid,	1	ITR(1, "setegid",	"d")	}, /*   136 */
   { svr4_sysconfig,	1	ITR(0, "sysconfig",	"d")	}, /*   137 */
   { 0,			Ukn	ITR(1, "adjtime",	"")	}, /*   138 */
   { svr4_sysinfo,	3	ITR(0, "systeminfo",	"dsd")	}, /*   139 */
   { socksys_syscall,	1	ITR(0, "socksys_syscall","x")	}, /*   140 */
   { svr4_seteuid,	1	ITR(1, "seteuid",	"d")	}, /*   141 */
};

EXPORT_SYMBOL(svr4_generic_funcs);
