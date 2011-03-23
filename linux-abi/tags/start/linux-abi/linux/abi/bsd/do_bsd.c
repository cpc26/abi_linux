#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <asm/unistd.h>

#include <abi/abi.h>
#include <abi/bsd.h>
#include <abi/map.h>
#include <abi/signal.h>

static ABI_func BSD_funcs[] = {
   { abi_syscall,	Fast	ITR(0, "syscall",	"")	}, /*    0 */
   { SC(exit),		-1	ITR(0, "exit",		"d")	}, /*    1 */
   { abi_fork,		Spl	ITR(0, "fork",		"")	}, /*    2 */
   { abi_read,		3	ITR(0, "read",		"dpd")	}, /*    3 */
   { SC(write),		-3	ITR(0, "write",		"dpd")	}, /*    4 */
   { bsd_open,		3	ITR(0, "open",		"soo")	}, /*    5 */
   { SC(close),		-1	ITR(0, "close",		"d")	}, /*    6 */
   { SC(wait4),		-4	ITR(0, "wait4",		"dxxx")	}, /*    7 */
   { SC(creat),		-2	ITR(0, "creat",		"so")	}, /*    8 */
   { SC(link),		-2	ITR(0, "link",		"ss")	}, /*    9 */
   { SC(unlink),	-1	ITR(0, "unlink",	"s")	}, /*   10 */
   { abi_exec,		Spl	ITR(0, "exec",		"sxx")	}, /*   11 */
   { SC(chdir),		-1	ITR(0, "chdir",		"s")	}, /*   12 */
   { SC(fchdir),	-1	ITR(0, "fchdir",	"d")	}, /*   13 */
   { abi_mknod,	3	ITR(0, "mknod",		"soo")	}, /*   14 */
   { SC(chmod),		-2	ITR(0, "chmod",		"so")	}, /*   15 */
   { SC(chown),		-3	ITR(0, "chown",		"sdd")	}, /*   16 */
   { abi_brk,		1	ITR(0, "brk/break",	"x")	}, /*   17 */
   { 0,			3	ITR(1, "getfsstat",	"xdd")	}, /*   18 */
   { SC(lseek),		-3	ITR(0, "seek/lseek",	"ddd")	}, /*   19 */
   { abi_getpid,	Spl	ITR(0, "getpid",	"")	}, /*   20 */
   { 0,			4	ITR(1, "mount",		"dsdx")	}, /*   21 */
   { 0,			2	ITR(0, "umount",	"sd")	}, /*   22 */
   { SC(setuid),	-1	ITR(0, "setuid",	"d")	}, /*   23 */
   { abi_getuid,	Spl	ITR(0, "getuid",	"")	}, /*   24 */
   { bsd_geteuid,	0	ITR(0, "geteuid",	"")	}, /*   25 */
   { SC(ptrace),	-4	ITR(0, "ptrace",	"xxxx")	}, /*   26 */
   { 0,			3	ITR(1, "recvmsg",	"dxd")	}, /*   27 */
   { 0,			3	ITR(1, "sendmsg",	"dxd")	}, /*   28 */
   { abi_recvfrom,	6	ITR(0, "recvfrom",	"dxddxd")},/*   29 */
   { abi_accept,	3	ITR(0, "accept",	"dxx")	}, /*   30 */
   { abi_getpeername,	3	ITR(0, "getpeername",	"dxx")	}, /*   31 */
   { abi_getsockname,	Spl	ITR(0, "getsockname",	"")	}, /*   32 */
   { SC(access),	-2	ITR(0, "access",	"so")	}, /*   33 */
   { 0,			2	ITR(1, "chflags",	"sx")	}, /*   34 */
   { 0,			2	ITR(1, "fchflags",	"dx")	}, /*   35 */
   { SC(sync),		-ZERO	ITR(0, "sync",		"")	}, /*   36 */
   { abi_kill,		2	ITR(0, "kill",		"dd")	}, /*   37 */
   { bsd_stat,		2	ITR(0, "stat",		"sp")	}, /*   38 */
   { SC(getppid),	0	ITR(0, "getppid",	"")	}, /*   39 */
   { bsd_lstat,		2	ITR(0, "lstat",		"sp")	}, /*   40 */
   { SC(dup),		-1	ITR(0, "dup",		"d")	}, /*   41 */
   { abi_pipe,		Spl	ITR(0, "pipe",		"")	}, /*   42 */
   { bsd_getegid,	0	ITR(0, "getegid",	"")	}, /*   43 */
   { SC(profil),	-4	ITR(0, "prof",		"xxxx")	}, /*   44 */
   { 0,			4	ITR(1, "ktrace",	"xddd")	}, /*   45 */
   { bsd_sigaction,	3	ITR(0, "sigaction",	"dxx")	}, /*   46 */
   { abi_getgid,	Spl	ITR(0, "getgid",	"")	}, /*   47 */
   { bsd_sigprocmask,	3	ITR(0, "sigprocmask",	"dxx")	}, /*   48 */
   { 0,			0	ITR(1, "getlogin",	"")	}, /*   49 */
   { 0,			1	ITR(1, "setlogin",	"1")	}, /*   50 */
   { SC(acct),		-1	ITR(0, "acct/sysacct",	"x")	}, /*   51 */
   { bsd_sigpending,	1	ITR(1, "sigpending",	"x")	}, /*   52 */
   { 0,			Ukn	ITR(1, "sigaltstack",	"")	}, /*   53 */
   { bsd_ioctl,		Spl	ITR(0, "ioctl",		"dxx")	}, /*   54 */
   { 0,			1	ITR(1, "reboot",	"x")	}, /*   55 */
   { 0,			Ukn	ITR(1, "revoke",	"")	}, /*   56 */
   { SC(symlink),	-2	ITR(0, "symlink",	"ss")	}, /*   57 */
   { SC(readlink),	-3	ITR(0, "readlink",	"spd")	}, /*   58 */
   { abi_exec,		Spl	ITR(0, "execv",		"spp")	}, /*   59 */
   { SC(umask),		-1	ITR(0, "umask",		"o")	}, /*   60 */
   { SC(chroot),	-1	ITR(0, "chroot",	"s")	}, /*   61 */
   { bsd_fstat,		2	ITR(0, "fstat",		"dp")	}, /*   62 */
   { 0,			4	ITR(1, "getkerninfo",	"dxxd")	}, /*   63 */
   { bsd_getpagesize,	0	ITR(0, "getpagesize",	"")	}, /*   64 */
   { 0,			2	ITR(1, "msync",		"xx")	}, /*   65 */
   { abi_fork,		Spl	ITR(0, "vfork",		"")	}, /*   66 */
   { 0,			Ukn	ITR(1, "vread",		"")	}, /*   67 */
   { 0,			Ukn	ITR(1, "vwrite",	"")	}, /*   68 */
   { bsd_sbrk,		1	ITR(0, "sbrk",		"d")	}, /*   69 */
   { 0,			Ukn	ITR(1, "sstk",		"")	}, /*   70 */
   { 0,			5	ITR(1, "mmap",		"xddddd")},/*   71 */
   { 0,			Ukn	ITR(1, "vadvise",	"")	}, /*   72 */
   { 0,			2	ITR(1, "munmap",	"xx")	}, /*   73 */
   { 0,			3	ITR(1, "mprotect",	"xxx")	}, /*   74 */
   { 0,			3	ITR(1, "madvise",	"xxd")	}, /*   75 */
   { SC(vhangup),	-ZERO	ITR(0, "vhangup",	"")	}, /*   76 */
   { 0,			Ukn	ITR(1, "vlimit",	"")	}, /*   77 */
   { 0,			3	ITR(1, "mincore",	"xxx")	}, /*   78 */
   { SC(getgroups),	-2	ITR(0, "getgroups",	"dx")	}, /*   79 */
   { SC(setgroups),	-2	ITR(0, "setgroups",	"dx")	}, /*   80 */
   { SC(getpgrp),	-ZERO	ITR(0, "getpgrp",	"")	}, /*   81 */
   { SC(setpgid),	-2	ITR(0, "setpgid",	"dd")	}, /*   82 */
   { SC(setitimer),	-3	ITR(0, "setitimer",	"dxx")	}, /*   83 */
   { SC(wait4),		-4	ITR(0, "wait4",		"dxxx")	}, /*   84 */
   { 0,			1	ITR(1, "swapon",	"s")	}, /*   85 */
   { SC(getitimer),	-2	ITR(0, "gettimer",	"dx")	}, /*   86 */
   { abi_gethostname,	2	ITR(0, "gethostname",	"xd")	}, /*   87 */
   { SC(sethostname),	-2	ITR(0, "sethostname",	"sd")	}, /*   88 */
   { bsd_getdtablesize,	0	ITR(0, "getdtablesize",	"")	}, /*   89 */
   { SC(dup2),		-2	ITR(0, "dup2",		"dd")	}, /*   90 */
   { 0,			Ukn	ITR(0, "?",		"")	}, /*   91 */
   { bsd_fcntl,		Spl	ITR(0, "fcntl",		"ddd")	}, /*   92 */
#ifdef CONFIG_ABI_TRACE
   { abi_select,	5	ITR(0, "select",	"dxxxx")}, /*   93 */
#else
   { SC(_newselect),	-5	ITR(0, "select",	"dxxxx")}, /*   93 */
#endif
   { 0,			Ukn	ITR(0, "?",		"")	}, /*   94 */
   { SC(fsync),		-1	ITR(0, "fsync",		"d")	}, /*   95 */
   { 0,			3	ITR(1, "setpriority",	"ddd")	}, /*   96 */
   { abi_socket,	Spl	ITR(0, "socket",	"ddd")	}, /*   97 */
   { bsd_connect,	Spl	ITR(0, "connect",	"dxd")	}, /*   98 */
   { abi_accept,	Spl	ITR(0, "accept",	"dxx")	}, /*   99 */
   { 0,			2	ITR(1, "getpriority",	"dd")	}, /*   100 */
   { abi_send,	Spl	ITR(0, "send",		"dxdd")	}, /*   101 */
   { abi_recv,	Spl	ITR(0, "recv",		"dxdd")	}, /*   102 */
   { 0,			1	ITR(1, "sigreturn",	"x")	}, /*   103 */
   { abi_bind,	Spl	ITR(0, "bind",		"dxd")	}, /*   104 */
   { abi_setsockopt,	Spl	ITR(0, "setsockopt",	"dddxd")}, /*   105 */
   { abi_listen,	Spl	ITR(0, "listen",	"dd")	}, /*   106 */
   { 0,			Ukn	ITR(1, "vtimes",	"")	}, /*   107 */
   { 0,			3	ITR(1, "sigvec",	"dxx")	}, /*   108 */
   { 0,			1	ITR(1, "sigblock",	"x")	}, /*   109 */
   { 0,			1	ITR(1, "sigsetmask",	"x")	}, /*   110 */
   { abi_sigsuspend,	Spl	ITR(0, "sigsuspend",	"x")	}, /*   111 */
   { 0,			2	ITR(1, "sigstack",	"xx")	}, /*   112 */
   { 0,			4	ITR(1, "recvmsg",	"dxdx")	}, /*   113 */
   { 0,			3	ITR(1, "sendmsg",	"dxx")	}, /*   114 */
   { 0,			2	ITR(1, "vtrace",	"dd")	}, /*   115 */
   { SC(gettimeofday),	-2	ITR(0, "gettimeofday",	"xx")	}, /*   116 */
   { SC(getrusage),	-2	ITR(0, "getrusage",	"dx")	}, /*   117 */
   { abi_getsockopt,	Spl	ITR(0, "getsockopt",	"")	}, /*   118 */
   { 0,			Ukn	ITR(1, "resuba",	"")	}, /*   119 */
#ifdef __NR_readv /* Around kernel 1.3.31 */
   { SC(readv),		-3	ITR(0, "readv",		"dxd")	}, /*   121 */
   { SC(writev),	-3	ITR(0, "writev",	"dxd")	}, /*   122 */
#else
   { 0,			Ukn	ITR(1, "readv",		"")	}, /*   121 */
   { abi_writev,	3	ITR(0, "writev",	"dxd")	}, /*   122 */
#endif
   { SC(settimeofday),	-2	ITR(0, "settimeofday",	"xx")	}, /*   122 */
   { SC(fchown),	-3	ITR(0, "fchown",	"ddd")	}, /*   123 */
   { SC(fchmod),     	-2	ITR(0, "fchmod",	"do")	}, /*   124 */
   { abi_recvfrom,	Spl	ITR(0, "recvfrom",	"dxddxd")},/*   125 */
   { SC(setreuid),	-2	ITR(0, "setreuid",	"dd")	}, /*   126 */
   { SC(setregid),	-2	ITR(0, "setregid",	"dd")	}, /*   127 */
   { SC(rename),	-2	ITR(0, "rename",	"ss")	}, /*   128 */
   { SC(truncate),	-2	ITR(0, "truncate",	"sd")	}, /*   129 */
   { SC(ftruncate),	-2	ITR(0, "ftruncate",	"dd")	}, /*   130 */
   { 0,			2	ITR(1, "flock",		"dd")	}, /*   131 */
   { 0,			2	ITR(1, "mkfifo",	"so")	}, /*   132 */
   { abi_sendto,	Spl	ITR(0, "sendto",	"dxddxd")},/*   133 */
   { abi_shutdown,	Spl	ITR(0, "shutdown",	"dd")	}, /*   134 */
   { abi_socketpair,	Spl	ITR(0, "socketpair",	"dddx")	}, /*   135 */
   { abi_mkdir,	2	ITR(0, "mkdir",		"so")	}, /*   136 */
   { SC(rmdir),		-1	ITR(0, "rmdir",		"s")	}, /*   137 */
   { 0,			Ukn	ITR(1, "utimes",	"")	}, /*   138 */
   { 0,			Ukn	ITR(1, "sigreturn",	"x")	}, /*   139 */
   { 0,			Ukn	ITR(1, "adjtime",	"xx")	}, /*   140 */
   { abi_getpeername,	Spl	ITR(0, "getpeername",	"dxx")	}, /*   141 */
   { 0,			0	ITR(1, "gethostid",	"")	}, /*   142 */
   { 0,			1	ITR(1, "sethostid",	"x")	}, /*   143 */
   { SC(getrlimit),	-2	ITR(0, "getrlimit",	"dx")	}, /*   144 */
   { SC(setrlimit),	-2	ITR(0, "setrlimit",	"dx")	}, /*   145 */
   { bsd_killpg,	2	ITR(0, "killpg",	"dd")	}, /*   146 */
   { SC(setsid),	-ZERO	ITR(0, "setsid",	"")	}, /*   147 */
   { 0,			Ukn	ITR(1, "quotactl",	"")	}, /*   148 */
   { 0,			Ukn	ITR(1, "quota",		"")	}, /*   149 */
   { abi_getsockname,	Spl	ITR(0, "getsockname",	"")	}, /*   150 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /*   151 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /*   152 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /*   153 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /*   154 */
   { 0,			1	ITR(1, "nfssvc",	"d")	}, /*   155 */
   { bsd_getdirentries,	4	ITR(0, "getdirentries",	"dxdx")	}, /*   156 */
   { bsd_statfs,	2	ITR(0, "statfs",	"sx")	}, /*   157 */
   { bsd_fstatfs,	2	ITR(0, "fstatfs",	"dx")	}, /*   158 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /*   159 */
   { 0,			0	ITR(1, "async_daemon",	"")	}, /*   160 */
   { 0,			2	ITR(1, "getfh",		"sx")	}, /*   161 */
   { abi_getdomainname,2	ITR(0, "getdomainname","xd")	}, /*   162 */
   { SC(setdomainname),	-2	ITR(0, "setdomainname","sd")	}, /*   163 */
   { 0,			Ukn	ITR(1, "uname",		"")	}, /*   164 */
   { 0,			Ukn	ITR(1, "sysarch",	"")	}, /*   165 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /*   166 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /*   167 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /*   168 */
   { 0,			Ukn	ITR(1, "semsys",	"")	}, /*   169 */
   { 0,			Ukn	ITR(1, "msgsys",	"")	}, /*   170 */
   { 0,			Ukn	ITR(1, "shmsys",	"")	}, /*   171 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /*   172 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /*   173 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /*   174 */
   { 0,			Ukn	ITR(1, "ntp_gettime",	"")	}, /*   175 */
   { 0,			Ukn	ITR(1, "ntp_adjtime",	"")	}, /*   176 */
   { 0,			Ukn	ITR(1, "vm_allocate",	"")	}, /*   177 */
   { 0,			Ukn	ITR(1, "vm_deallocate",	"")	}, /*   178 */
   { 0,			Ukn	ITR(1, "vm_inherit",	"")	}, /*   179 */
   { 0,			Ukn	ITR(1, "vm_protect",	"")	}, /*   180 */
   { SC(setgid),	-1	ITR(0, "setgid",	"d")	}, /*   181 */
   { bsd_setegid,	1	ITR(0, "setegid",	"d")	}, /*   182 */
   { bsd_seteuid,	1	ITR(0, "seteuid",	"d")	}  /*   183 */
};


static void BSD_lcall7(int segment, struct pt_regs * regs)
{
	int i = regs->eax & 0xff;

	if (i < 183)
		abi_dispatch(regs, &BSD_funcs[i], 1);
	else {
		regs->eax = iABI_errors(-EINVAL);
		regs->eflags |= 1;
	}
}

static unsigned char BSD_err_table1[] = {
	11, 63, 77, 78, 66, 62, 35
};

static unsigned char BSD_err_table2[] = {
	68, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52,
	53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 65, 37, 36, 70, EINVAL,
	EINVAL, EINVAL, EINVAL, EINVAL, 69
};

static struct map_segment BSD_err_map[] = {
	{ 11,	11,				(unsigned char *)35 },
	{ 35,	35+sizeof(BSD_err_table1)-1,	BSD_err_table1 },
	{ 66,	66,				(unsigned char *)71 },
	{ 87,	87+sizeof(BSD_err_table2)-1,	BSD_err_table2 },
	{ 0,	1000,				NULL },
	{ -1 }
};

static long linux_to_bsd_signals[NSIGNALS+1] = {
	0,
	BSD_SIGHUP,	BSD_SIGINT,	BSD_SIGQUIT,	BSD_SIGILL,
	BSD_SIGTRAP,	BSD_SIGABRT,	BSD_SIGEMT,	BSD_SIGFPE,
	BSD_SIGKILL,	BSD_SIGUSR1,	BSD_SIGSEGV,	BSD_SIGUSR2,
	BSD_SIGPIPE,	BSD_SIGALRM,	BSD_SIGTERM,	BSD_SIGSEGV,
	BSD_SIGCHLD,	BSD_SIGCONT,	BSD_SIGSTOP,	BSD_SIGTSTP,
	BSD_SIGTTIN,	BSD_SIGTTOU,	BSD_SIGIO,	BSD_SIGXCPU,
	BSD_SIGXFSZ,	BSD_SIGVTALRM,	BSD_SIGPROF,	BSD_SIGWINCH,
	-1,		BSD_SIGTERM,	-1,		-1
};

static long bsd_to_linux_signals[NSIGNALS+1] = {
	0,
	SIGHUP,		SIGINT,		SIGQUIT,	SIGILL,
	SIGTRAP,	SIGABRT,	SIGUNUSED,	SIGFPE,
	SIGKILL,	SIGUNUSED,	SIGSEGV,	SIGUNUSED,
	SIGPIPE,	SIGALRM,	SIGTERM,	SIGURG,
	SIGSTOP,	SIGTSTP,	SIGCONT,	SIGCHLD,
	SIGTTIN,	SIGTTOU,	SIGIO,		SIGXCPU,
	SIGXFSZ,	SIGVTALRM,	SIGPROF,	SIGWINCH,
	29,		SIGUSR1,	SIGUSR2,	-1
};

extern struct map_segment abi_sockopt_map[];
extern struct map_segment abi_af_map[];


struct exec_domain bsd_exec_domain = {
	"BSD",
	BSD_lcall7,
	PER_BSD & PER_MASK, PER_BSD & PER_MASK,
	bsd_to_linux_signals,
	linux_to_bsd_signals,
	BSD_err_map,
	NULL,
	abi_sockopt_map,
	abi_af_map,
#ifdef MODULE
	&__this_module,
#else
	NULL,
#endif
	NULL
};

static void __exit bsd_cleanup(void)
{
  unregister_exec_domain(&bsd_exec_domain);
}

static int __init bsd_init(void)
{
  return register_exec_domain(&bsd_exec_domain);
}

module_init(bsd_init);
module_exit(bsd_cleanup);
