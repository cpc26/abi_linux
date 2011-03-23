/*
 *    abi/uw7/funcs.c - UnixWare 7.x system call dispatch table.
 *
 *  This software is under GPL
 */

#include <abi/abi.h>
#include <abi/abi4.h>
#include <abi/svr4.h>
#include <abi/uw7.h>
#include <abi/uw7_context.h>

extern void iBCS_class_XNX(struct pt_regs *regs);

ABI_func uw7_funcs[] = {
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
   { abi_mknod,		3	ITR(0, "mknod",		"soo")	}, /*   14 */
   { SC(chmod),		-2	ITR(0, "chmod",		"so")	}, /*   15 */
   { SC(chown),		-3	ITR(0, "chown",		"sdd")	}, /*   16 */
   { abi_brk,		1	ITR(0, "brk/break",	"x")	}, /*   17 */
   { abi_stat,		2	ITR(0, "stat",		"sp")	}, /*   18 */
   { abi_lseek,		3	ITR(0, "seek/lseek",	"ddd")	}, /*   19 */
   { abi_getpid,	Spl	ITR(0, "getpid",	"")	}, /*   20 */
   { 0,			Ukn	ITR(1, "mount",		"")	}, /*   21 */
   { SC(umount),	-1	ITR(0, "umount",	"s")	}, /*   22 */
   { SC(setuid),	-1	ITR(0, "setuid",	"d")	}, /*   23 */
   { abi_getuid,	Spl	ITR(0, "getuid",	"")	}, /*   24 */
   { SC(stime),		-1	ITR(0, "stime",		"d")	}, /*   25 */
   { svr4_ptrace,	4	ITR(0, "ptrace",	"xdxx") }, /*   26 */
   { SC(alarm),		-1	ITR(0, "alarm",		"d")	}, /*   27 */
   { abi_fstat,		2	ITR(0, "fstat",		"dp")	}, /*   28 */
   { SC(pause),		-ZERO	ITR(0, "pause",		"")	}, /*   29 */
   { SC(utime),		-2	ITR(0, "utime",		"xx")	}, /*   30 */
   { uw7_stty,		2	ITR(0, "stty",		"dd")	}, /*   31 */
   { uw7_gtty,		2	ITR(1, "gtty",		"dd")	}, /*   32 */
   { uw7_access,	2	ITR(0, "access",	"so")	}, /*   33 */
   { SC(nice),		-1	ITR(0, "nice",		"d")	}, /*   34 */
   { svr4_statfs,	4	ITR(0, "statfs",	"spdd")	}, /*   35 */
   { SC(sync),		-ZERO	ITR(0, "sync",		"")	}, /*   36 */
   { abi_kill,		2	ITR(0, "kill",		"dd")	}, /*   37 */
   { svr4_fstatfs,	4	ITR(0, "fstatfs",	"dpdd")	}, /*   38 */
   { abi_procids,	Spl	ITR(0, "ibcs_procids",	"d")	}, /*   39 */
   { iBCS_class_XNX,	Fast	ITR(0, "sysext",	"")	}, /*   40 */
   { SC(dup),		-1	ITR(0, "dup",		"d")	}, /*   41 */
   { abi_pipe,		Spl	ITR(0, "pipe",		"")	}, /*   42 */
   { SC(times),		-1	ITR(0, "times",		"p")	}, /*   43 */
   { SC(profil),	-4	ITR(0, "prof",		"xxxx")},  /*   44 */
   { 0,			Ukn	ITR(1, "lock/plock",	"")	}, /*   45 */
   { SC(setgid),	-1	ITR(0, "setgid",	"d")	}, /*   46 */
   { abi_getgid,	Spl	ITR(0, "getgid",	"")	}, /*   47 */
   { abi_sigfunc,	Fast	ITR(0, "sigfunc",	"xxx")	}, /*   48 */
   { svr4_msgsys,	Spl	ITR(0, "msgsys",	"dxddd")}, /*   49 */
   { svr4_sysi86,	Spl	ITR(0, "sysi86/sys3b",	"d")	}, /*   50 */
   { SC(acct),		-1	ITR(0, "acct/sysacct",	"x")	}, /*   51 */
   { svr4_shmsys,	Fast	ITR(0, "shmsys",	"ddxo")},  /*   52 */
   { svr4_semsys,	Spl	ITR(0, "semsys",	"dddx")},  /*   53 */
   { uw7_ioctl,		Spl	ITR(0, "ioctl",		"dxx")	}, /*   54 */
   { 0,			3	ITR(0, "uadmin",	"xxx")	}, /*   55 */
   { 0,			Ukn	ITR(1, "exch",		"")	}, /*   56 */
   { v7_utsname,	1	ITR(0, "utsys",		"x")	}, /*   57 */
   { SC(fsync),		-1	ITR(0, "fsync",		"d")	}, /*   58 */
   { abi_exec,		Spl	ITR(0, "execv",		"spp")	}, /*   59 */
   { SC(umask),		-1	ITR(0, "umask",		"o")	}, /*   60 */
   { SC(chroot),	-1	ITR(0, "chroot",	"s")	}, /*   61 */
   { svr4_fcntl,	Spl	ITR(0, "fcntl",		"dxx")	}, /*   62 */
   { svr4_ulimit,	2	ITR(0, "ulimit",	"xx")	}, /*   63 */
   { 0,			Ukn	ITR(1, "cg_ids",	"")	}, /*   64 */
   { 0,			Ukn	ITR(1, "cg_processors",	"")	}, /*   65 */
   { 0,			Ukn	ITR(1, "cg_info",	"")	}, /*   66 */
   { 0,			Ukn	ITR(1, "cg_bind",	"")	}, /*   67 */
   { 0,			Ukn	ITR(1, "cg_current",	"")	}, /*   68 */
   { 0,			Ukn	ITR(1, "cg_memloc",	"")	}, /*   69 */
   { 0,			Ukn	ITR(1, "advfs",		"")	}, /*   70 */
   { 0,			Ukn	ITR(1, "unadvfs",	"")	}, /*   71 */
   { 0,			Ukn	ITR(1, "rmount",	"")	}, /*   72 */
   { 0,			Ukn	ITR(1, "rumount",	"")	}, /*   73 */
   { 0,			Ukn	ITR(1, "rfstart",	"")	}, /*   74 */
   { 0,			Ukn	ITR(1, "unused 75",	"")	}, /*   75 */
   { 0,			Ukn	ITR(1, "rdebug",	"")	}, /*   76 */
   { 0,			Ukn	ITR(1, "rfstop",	"")	}, /*   77 */
   { 0,			Ukn	ITR(1, "rfsys",		"")	}, /*   78 */
   { SC(rmdir),		-1	ITR(0, "rmdir",		"s")	}, /*   79 */
   { abi_mkdir,		2	ITR(0, "mkdir",		"so")	}, /*   80 */
   { svr4_getdents,	3	ITR(0, "getdents",	"dxd")	}, /*   81 */
   { 0,			Ukn	ITR(1, "libattach",	"")	}, /*   82 */
   { 0,			Ukn	ITR(1, "libdetach",	"")	}, /*   83 */
   { svr4_sysfs,	3	ITR(0, "sysfs",		"dxx")	}, /*   84 */
   { svr4_getmsg,	Spl	ITR(0, "getmsg",	"dxxx")	}, /*   85 */
   { svr4_putmsg,	Spl	ITR(0, "putmsg",	"dxxd")	}, /*   86 */
   { svr4_poll,		3	ITR(0, "poll",		"xdd")	}, /*   87 */
   { abi_lstat,		2	ITR(0, "lstat",		"sp")	}, /*   88 */
   { SC(symlink),	-2	ITR(0, "symlink",	"ss")	}, /*   89 */
   { SC(readlink),	-3	ITR(0, "readlink",	"spd")	}, /*   90 */
   { 0,			Ukn	ITR(0, "svr4_setgroups","dp")	}, /*   91 */
   { 0,			Ukn	ITR(0, "svr4_getgroups","dp")	}, /*   92 */
   { SC(fchmod),	-2	ITR(0, "fchmod",	"do")	}, /*   93 */
   { SC(fchown),	-3	ITR(0, "fchown",	"ddd")	}, /*   94 */
   { abi_sigprocmask,	3	ITR(0, "sigprocmask",	"dxx")	}, /*   95 */
   { abi_sigsuspend,	Spl	ITR(0, "sigsuspend",	"x")	}, /*   96 */
   { uw7_sigaltstack,	2	ITR(1, "sigaltstack",	"xx")	}, /*   97 */
   { abi_sigaction,	3	ITR(0, "sigaction",	"dxx")	}, /*   98 */
   { svr4_sigpending,	2	ITR(1, "sigpending",	"dp")	}, /*   99 */
   { uw7_context,	Spl	ITR(0, "ucontext",	"")	}, /*   100 */
   { 0,			Ukn	ITR(1, "evsys",		"")	}, /*   101 */
   { 0,			Ukn	ITR(1, "evtrapret",	"")	}, /*   102 */
   { abi_statvfs,	2	ITR(0, "statvfs",	"sp")	}, /*   103 */
   { abi_statvfs,	2	ITR(0, "fstatvfs",	"dp")	}, /*   104 */
   { 0,			Ukn	ITR(1, "reserved 105",	"")	}, /*	105 */
   { 0,			Ukn	ITR(1, "nfssys",	"")	}, /*   106 */
   { svr4_waitid,	4	ITR(0, "waitid",	"ddxd")	}, /*   107 */
   { 0,			3	ITR(1, "sigsendsys",	"ddd")	}, /*   108 */
   { svr4_hrtsys,	Spl	ITR(0, "hrtsys",	"xxx")	}, /*   109 */
   { 0,			3	ITR(1, "acancel",	"dxd")	}, /*   110 */
   { 0,			Ukn	ITR(1, "async",		"")	}, /*   111 */
   { 0,			Ukn	ITR(1, "priocntlsys",	"")	}, /*   112 */
   { svr4_pathconf,	2	ITR(1, "pathconf",	"sd")	}, /*   113 */
   { 0,			3	ITR(1, "mincore",	"xdx")	}, /*   114 */
   { uw7_mmap,		6	ITR(0, "mmap",		"xxxxdx") },/*   115 */
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
   { svr4_xmknod,	4	ITR(0, "xmknod",	"dsox")},  /*   126 */
   { svr4_syslocal,	Spl	ITR(0, "syslocal",	"d")	}, /*   127 */
   { svr4_getrlimit,	2	ITR(0, "setrlimit",	"dx")	}, /*   128 */
   { svr4_setrlimit,	2	ITR(0, "getrlimit",	"dx")	}, /*   129 */
   { SC(lchown),	3	ITR(1, "lchown","sdd")	},         /*   130 */
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
   { uw7_setegid,	1	ITR(1, "setegid",	"d")	}, /*   136 */
   { svr4_sysconfig,	1	ITR(0, "sysconfig",	"d")	}, /*   137 */
   { 0,			Ukn	ITR(1, "adjtime",	"")	}, /*   138 */
   { svr4_sysinfo,	3	ITR(0, "systeminfo",	"dsd")	}, /*   139 */
   { socksys_syscall,	1	ITR(0, "socksys_syscall","x")	}, /*   140 */
   { uw7_seteuid,	1	ITR(1, "seteuid",	"d")	}, /*   141 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /*   142 */
   { 0,			Ukn	ITR(1, "keyctl",	"")	}, /*   143 */
   { 0,			2	ITR(1, "secsys",	"dx")	}, /*	144 */
   { 0,			4	ITR(1, "filepriv",	"sdxd")	}, /*	145 */
   { 0,			3	ITR(1, "procpriv",	"dxd")	}, /*	146 */
   { 0,			3	ITR(1, "devstat",	"sdx")	}, /*	147 */
   { 0,			5	ITR(1, "aclipc",	"ddddx")}, /*	148 */
   { 0,			3	ITR(1, "fdevstat",	"ddx")	}, /*	149 */
   { 0,			3	ITR(1, "flvlfile",	"ddx")	}, /*	150 */
   { 0,			3	ITR(1, "lvlfile",	"sdx")	}, /*	151 */
   { 0,			Ukn	ITR(1, "sendv",		"")	}, /*	152 */
   { 0,			2	ITR(1, "lvlequal",	"xx")	}, /*	153 */
   { 0,			2	ITR(1, "lvlproc",	"dx")	}, /*	154 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /*	155 */
   { 0,			4	ITR(1, "lvlipc",	"dddx")	}, /*	156 */
   { 0,			4	ITR(1, "acl",		"sddx")	}, /*	157 */
   { 0,			Ukn	ITR(1, "auditevt",	"")	}, /*	158 */
   { 0,			Ukn	ITR(1, "auditctl",	"")	}, /*	159 */
   { 0,			Ukn	ITR(1, "auditdmp",	"")	}, /*	160 */
   { 0,			Ukn	ITR(1, "auditlog",	"")	}, /*	161 */
   { 0,			Ukn	ITR(1, "auditbuf",	"")	}, /*	162 */
   { 0,			2	ITR(1, "lvldom",	"xx")	}, /*	163 */
   { 0,			Ukn	ITR(1, "lvlvfs",	"")	}, /*	164 */
   { 0,			2	ITR(1, "mkmld",		"so")	}, /*	165 */
   { uw7_mldmode,	1	ITR(1, "mldmode",	"d")	}, /*	166 */
   { 0,			2	ITR(0, "secadvise",	"xx")	}, /*	167 */
   { 0,			Ukn	ITR(1, "online",	"")	}, /*	168 */
   { SC(setitimer),	-3	ITR(0, "setitimer",	"dxx")	}, /*	169 */
   { SC(getitimer),	-2	ITR(0, "getitimer",	"dx")	}, /*	170 */
   { SC(gettimeofday),	-2	ITR(0, "gettimeofday",	"xx")	}, /*	171 */
   { SC(settimeofday),	-2	ITR(0, "settimeofday",	"xx")	}, /*	172 */
   { 0,			Ukn	ITR(1, "lwpcreate",	"")	}, /*	173 */
   { 0,			Ukn	ITR(1, "lwpexit",	"")	}, /*	174 */
   { 0,			Ukn	ITR(1, "lwpwait",	"")	}, /*	175 */
   { 0,			Ukn	ITR(1, "lwpself",	"")	}, /*	176 */
   { 0,			Ukn	ITR(1, "lwpinfo",	"")	}, /*	177 */
   { 0,			Ukn	ITR(1, "lwpprivate",	"")	}, /*	178 */
   { 0,			Ukn	ITR(1, "processorbind",	"")	}, /*	179 */
   { 0,			Ukn	ITR(1, "processorexbind","")	}, /*	180 */
   { 0,			Ukn	ITR(1, "",		"")	}, /*	181 */
   { 0,			Ukn	ITR(1, "sync_mailbox",	"")	}, /*	182 */
   { 0,			Ukn	ITR(1, "prepblock",	"")	}, /*	183 */
   { 0,			Ukn	ITR(1, "block",		"")	}, /*	184 */
   { 0,			Ukn	ITR(1, "rdblock",	"")	}, /*	185 */
   { 0,			Ukn	ITR(1, "unblock",	"")	}, /*	186 */
   { 0,			Ukn	ITR(1, "cancelblock",	"")	}, /*	187 */
   { 0,			Ukn	ITR(1, "187",		"")	}, /*	188 */
   { uw7_pread,		4	ITR(1, "pread",		"dsdd")	}, /*	189 */
   { uw7_pwrite,	4	ITR(1, "pwrite",	"dsdd")	}, /*	190 */
   { SC(truncate),	-2	ITR(0, "truncate",	"sd")	}, /*	191 */
   { SC(ftruncate),	-2	ITR(0, "ftruncate",	"dd")	}, /*	192 */
   { 0,			Ukn	ITR(1, "lwpkill",	"")	}, /*	193 */
   { 0,			Ukn	ITR(1, "sigwait",	"")	}, /*	194 */
   { abi_fork,		Spl	ITR(1, "fork1",		"")	}, /*	195 */
   { abi_fork,		Spl	ITR(1, "forkall",	"")	}, /*	196 */
   { 0,			Ukn	ITR(1, "modload",	"")	}, /*	197 */
   { 0,			Ukn	ITR(1, "moduload",	"")	}, /*	198 */
   { 0,			Ukn	ITR(1, "modpath",	"")	}, /*	199 */
   { 0,			Ukn	ITR(1, "modstat",	"")	}, /*	200 */
   { 0,			Ukn	ITR(1, "modadm",	"")	}, /*	201 */
   { 0,			Ukn	ITR(1, "getksym",	"")	}, /*	202 */
   { 0,			Ukn	ITR(1, "lwpsuspend",	"")	}, /*	203 */
   { 0,			Ukn	ITR(1, "lwpcontinue",	"")	}, /*	204 */
   { 0,			Ukn	ITR(1, "priocntllst",	"")	}, /*	205 */
   { uw7_sleep,		1	ITR(1, "sleep",		"d")	}, /*	206 */
   { 0,			Ukn	ITR(1, "lwp_sema_wait",	"")	}, /*	207 */
   { 0,			Ukn	ITR(1, "lwp_sema_post",	"")	}, /*	208 */
   { 0,			Ukn	ITR(1, "lwp_sema_trywait","")	}, /*	209 */
   { 0,			Ukn	ITR(1, "reserved 210","")	}, /*	210 */
   { 0,			Ukn	ITR(1, "unused 211","")		}, /*	211 */
   { 0,			Ukn	ITR(1, "unused 212","")		}, /*	212 */
   { 0,			Ukn	ITR(1, "unused 213","")		}, /*	213 */
   { 0,			Ukn	ITR(1, "unused 214","")		}, /*	214 */
   { 0,			Ukn	ITR(1, "unused 215","")		}, /*	215 */
   { uw7_fstatvfs64,	2	ITR(1, "fstatvfs64",	"dp")	}, /*	216 */
   { uw7_statvfs64,	2	ITR(1, "statvfs64",	"sp")	}, /*	217 */
   { 0,			Ukn	ITR(1, "ftruncate64","")	}, /*	218 */
   { 0,			Ukn	ITR(1, "truncate64","")		}, /*	219 */
   { 0,			Ukn	ITR(1, "getrlimit64","")	}, /*	220 */
   { 0,			Ukn	ITR(1, "setrlimit64","")	}, /*	221 */
   { uw7_lseek64,	4	ITR(1, "lseek64",	"dddd")	}, /*	222 */
   { 0,			Ukn	ITR(1, "mmap64","")		}, /*	223 */
   { uw7_pread64,	5	ITR(1, "pread64",	"dsddd")}, /*	224 */
   { uw7_pwrite64,	5	ITR(1, "pwrite64",	"dsddd")}, /*	225 */
   { 0,			Ukn	ITR(1, "creat64","")		}, /*	226 */
   { 0,			Ukn	ITR(1, "dshmsys","")		}, /*	227 */
   { 0,			Ukn	ITR(1, "invlpg","")		}, /*	228 */
   { 0,			Ukn	ITR(1, "rfork1","")		}, /*	229 */
   { 0,			Ukn	ITR(1, "rforkall","")		}, /*	230 */
   { 0,			Ukn	ITR(1, "rexecve","")		}, /*	231 */
   { 0,			Ukn	ITR(1, "migrate","")		}, /*	232 */
   { 0,			Ukn	ITR(1, "kill3","")		}, /*	233 */
   { 0,			Ukn	ITR(1, "ssisys","")		}, /*	234 */
   { 0,			Ukn	ITR(1, "xaccept","")		}, /*	235 */
   { 0,			Ukn	ITR(1, "xbind","")		}, /*	236 */
   { 0,			Ukn	ITR(1, "xbindresvport","")	}, /*	237 */
   { 0,			Ukn	ITR(1, "xconnect","")		}, /*	238 */
   { 0,			Ukn	ITR(1, "xgetsockaddr","")	}, /*	239 */
   { 0,			Ukn	ITR(1, "xgetsockopt","")	}, /*	240 */
   { 0,			Ukn	ITR(1, "xlisten","")		}, /*	241 */
   { 0,			Ukn	ITR(1, "xrecvmsg","")		}, /*	242 */
   { 0,			Ukn	ITR(1, "xsendmsg","")		}, /*	243 */
   { 0,			Ukn	ITR(1, "xsetsockaddr","")	}, /*	244 */
   { 0,			Ukn	ITR(1, "xsetsockopt","")	}, /*	245 */
   { 0,			Ukn	ITR(1, "xshutdown","")		}, /*	246 */
   { 0,			Ukn	ITR(1, "xsocket","")		}, /*	247 */
   { 0,			Ukn	ITR(1, "xsocketpair","")	}, /*	248 */
   { 0,			Ukn	ITR(1, "unused 249","")		}, /*	249 */
   { 0,			Ukn	ITR(1, "unused 250","")		}, /*	250 */
};
