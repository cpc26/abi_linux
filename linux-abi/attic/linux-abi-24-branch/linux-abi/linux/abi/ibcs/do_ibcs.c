#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <abi/abi.h>
#include <abi/signal.h>

EXPORT_NO_SYMBOLS;

#ifdef CONFIG_ABI_IBCS_SCO
MODULE_PARM(sco_serial, "1-10s");
MODULE_PARM_DESC(sco_serial, "SCO Serial Number");
#endif

extern ABI_func svr4_generic_funcs[];

extern void iBCS_class_WYSETCP(struct pt_regs *regs);
extern void iBCS_class_WYSENFS(struct pt_regs *regs);

static ABI_func iBCS_funcs[] = {
   { 0,			Ukn	ITR(1, "?",		"")	}, /*   142 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /*   143 */
   { 0,			2	ITR(1, "secsys",	"dx")	}, /*	144 */
   { 0,			4	ITR(1, "filepriv",	"sdxd")	}, /*	145 */
   { 0,			3	ITR(1, "procpriv",	"dxd")	}, /*	146 */
   { 0,			3	ITR(1, "devstat",	"sdx")	}, /*	147 */
   { 0,			5	ITR(1, "aclipc",	"ddddx")}, /*	148 */
   { 0,			3	ITR(1, "fdevstat",	"ddx")	}, /*	149 */
   { 0,			3	ITR(1, "flvlfile",	"ddx")	}, /*	150 */
   { 0,			3	ITR(1, "lvlfile",	"sdx")	}, /*	151 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /*	152 */
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
   { 0,			Ukn	ITR(1, "mlddone",	"")	}, /*	166 */
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
   { 0,			Ukn	ITR(1, "?",		"")	}, /*	188 */
   { 0,			Ukn	ITR(1, "pread",		"")	}, /*	189 */
   { 0,			Ukn	ITR(1, "pwrite",	"")	}, /*	190 */
   { SC(truncate),	-2	ITR(0, "truncate",	"sd")	}, /*	191 */
   { SC(ftruncate),	-2	ITR(0, "ftruncate",	"dd")	}, /*	192 */
   { 0,			Ukn	ITR(1, "lwpkill",	"")	}, /*	193 */
   { 0,			Ukn	ITR(1, "sigwait",	"")	}, /*	194 */
   { 0,			Ukn	ITR(1, "fork1",		"")	}, /*	195 */
   { 0,			Ukn	ITR(1, "forkall",	"")	}, /*	196 */
   { 0,			Ukn	ITR(1, "modload",	"")	}, /*	197 */
   { 0,			Ukn	ITR(1, "moduload",	"")	}, /*	198 */
   { 0,			Ukn	ITR(1, "modpath",	"")	}, /*	199 */
   { 0,			Ukn	ITR(1, "modstat",	"")	}, /*	200 */
   { 0,			Ukn	ITR(1, "modadm",	"")	}, /*	201 */
   { 0,			Ukn	ITR(1, "getksym",	"")	}, /*	202 */
   { 0,			Ukn	ITR(1, "lwpsuspend",	"")	}, /*	203 */
   { 0,			Ukn	ITR(1, "lwpcontinue",	"")	}, /*	204 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /*	205 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /*	206 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /*	207 */
   { 0,			Ukn	ITR(1, "?",		"")	},
   { 0,			Ukn	ITR(1, "?",		"")	},
   { 0,			Ukn	ITR(1, "?",		"")	},
   { 0,			Ukn	ITR(1, "?",		"")	},
   { 0,			Ukn	ITR(1, "?",		"")	},
   { 0,			Ukn	ITR(1, "?",		"")	},
   { iBCS_class_WYSETCP,Fast	ITR(1, "?",		"")	},
   { 0,			Ukn	ITR(1, "?",		"")	}
};

#ifdef CONFIG_ABI_IBCS_SCO
static ABI_func SCO_funcs[] = {
   { 0,			Ukn	ITR(1, "?",		"")	}, /*   88 */
   { sw_security,	6	ITR(0, "security",	"dxxxxx")},/*   89 */
   { SC(symlink),	-2	ITR(0, "symlink",	"ss")	}, /*   90 */
   { abi_lstat,	2	ITR(0, "lstat",		"sp")	}, /*   91 */
   { SC(readlink),	-3	ITR(0, "readlink",	"spd")	}, /*   92 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /*   93 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /*   94 */
   { 0,			Ukn	ITR(1, "?",		"")	}  /*   95 */
};
#endif
#ifdef CONFIG_ABI_IBCS_WYSE
static ABI_func WYSE_funcs[] = {
   { abi_lstat,	2	ITR(0, "lstat",		"sp")	}, /*   128 */
   { SC(readlink),	-3	ITR(0, "readlink",	"spd")	}, /*   129 */
   { SC(symlink),	-2	ITR(0, "symlink",	"ss")	}, /*   130 */
   { iBCS_class_WYSETCP,Fast	ITR(0, "?",		"")	}, /*   131 */
   { iBCS_class_WYSENFS,Fast	ITR(0, "?",		"")	}, /*   132 */
   { abi_gethostname,	2	ITR(0, "gethostname",	"xd")	}, /*   133 */
   { SC(sethostname),	-2	ITR(0, "sethostname",	"sd")	}, /*   134 */
   { abi_getdomainname,2	ITR(0, "getdomainname","xd")	}, /*   135 */
   { SC(setdomainname),	-2	ITR(0, "setdomainname","sd")	}, /*   136 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /*   137 */
   { SC(setreuid),	-2	ITR(0, "setreuid",	"dd")	}, /*   138 */
   { SC(setregid),	-2	ITR(0, "setregid",	"dd")	}, /*   139 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /*   140 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /*   141 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /*   142 */
   { 0,			Ukn	ITR(1, "?",		"")	}  /*   143 */
};
#endif
static void iBCS_lcall7(int segment, struct pt_regs * regs)
{
	int i = regs->eax & 0xff;
	ABI_func *p;
	struct exec_domain *def;

	if (segment == 0x27) {
	  /* This is a Solaris binary, not SVR4. The ELF loader
	     can't tell the difference, though. Let the default
	     handler go looking for the Solaris module for us.
	  */

	  def = lookup_exec_domain(PER_LINUX);
	  def->handler(segment, regs);
	  return;
	}

#ifdef CONFIG_ABI_IBCS_WYSE
	if (i < 0x88 && i > 0x77 && current->personality == PER_WYSEV386) {
		p = &WYSE_funcs[i - 0x78];
	} 
	else
#endif
#ifdef CONFIG_ABI_IBCS_SCO
	if (i < 0x60 && i > 0x57 && current->personality == PER_SCOSVR3) {
		p = &SCO_funcs[i - 0x58];
	}
	else
#endif
	if (i < 142) {
		p = &svr4_generic_funcs[i];
	}
	else
		p = &iBCS_funcs[i - 142];

	abi_dispatch(regs, p, 1);
}

extern struct map_segment svr4_err_map[];
extern struct map_segment svr4_socktype_map[];
extern struct map_segment abi_sockopt_map[];
extern struct map_segment abi_af_map[];

#ifdef CONFIG_ABI_IBCS_SCO
static long linux_to_sco_signals[NSIGNALS+1] = {
	0,
	IBCS_SIGHUP,	IBCS_SIGINT,	IBCS_SIGQUIT,	IBCS_SIGILL,
	IBCS_SIGTRAP,	IBCS_SIGABRT,	-1,		IBCS_SIGFPE,
	IBCS_SIGKILL,	IBCS_SIGUSR1,	IBCS_SIGSEGV,	IBCS_SIGUSR2,
	IBCS_SIGPIPE,	IBCS_SIGALRM,	IBCS_SIGTERM,	IBCS_SIGSEGV,
	IBCS_SIGCHLD,	IBCS_SIGCONT,	IBCS_SIGSTOP,	IBCS_SIGTSTP,
	IBCS_SIGTTIN,	IBCS_SIGTTOU,	IBCS_SIGUSR1,	IBCS_SIGGXCPU,
	IBCS_SIGGXFSZ,	IBCS_SIGVTALRM,	IBCS_SIGPROF,	IBCS_SIGWINCH,
	IBCS_SIGIO,	IBCS_SIGPWR,	-1,		-1
};

static long sco_to_linux_signals[NSIGNALS+1] = {
	0,
	SIGHUP,		SIGINT,		SIGQUIT,	SIGILL,
	SIGTRAP,	SIGIOT,		SIGUNUSED,	SIGFPE,
	SIGKILL,	SIGUNUSED,	SIGSEGV,	SIGUNUSED,
	SIGPIPE,	SIGALRM,	SIGTERM,	SIGURG,
	SIGUSR2,	SIGCHLD,	SIGPWR,		SIGWINCH,
	SIGUNUSED,	SIGPOLL,	SIGSTOP,	SIGTSTP,
	SIGCONT,	SIGTTIN,	SIGTTOU,	SIGVTALRM,
	SIGPROF,	SIGXCPU,	SIGXFSZ,	-1
};
#endif

extern long linux_to_ibcs_signals[];
extern long ibcs_to_linux_signals[];


#ifdef CONFIG_ABI_IBCS_ISC
static long linux_to_isc_signals[NSIGNALS+1] = {
	0,
	IBCS_SIGHUP,	IBCS_SIGINT,	IBCS_SIGQUIT,	IBCS_SIGILL,
	IBCS_SIGTRAP,	IBCS_SIGABRT,	-1,		IBCS_SIGFPE,
	IBCS_SIGKILL,	IBCS_SIGUSR1,	IBCS_SIGSEGV,	IBCS_SIGUSR2,
	IBCS_SIGPIPE,	IBCS_SIGALRM,	IBCS_SIGTERM,	IBCS_SIGSEGV,
	IBCS_SIGCHLD,	ISC_SIGCONT,	ISC_SIGSTOP,	ISC_SIGTSTP,
	IBCS_SIGTTIN,	IBCS_SIGTTOU,	IBCS_SIGUSR1,	IBCS_SIGGXCPU,
	IBCS_SIGGXFSZ,	IBCS_SIGVTALRM,	IBCS_SIGPROF,	IBCS_SIGWINCH,
	IBCS_SIGIO,	IBCS_SIGPWR,	-1,		-1
};

static long isc_to_linux_signals[NSIGNALS+1] = {
	0,
	SIGHUP,		SIGINT,		SIGQUIT,	SIGILL,
	SIGTRAP,	SIGIOT,		SIGUNUSED,	SIGFPE,
	SIGKILL,	SIGUNUSED,	SIGSEGV,	SIGUNUSED,
	SIGPIPE,	SIGALRM,	SIGTERM,	SIGUSR1,
	SIGUSR2,	SIGCHLD,	SIGPWR,		SIGWINCH,
	-1,		SIGPOLL,	SIGCONT,	SIGSTOP,
	SIGTSTP,	SIGTTIN,	SIGTTOU,	SIGVTALRM,
	SIGPROF,	SIGXCPU,	SIGXFSZ,	-1
};
#endif /* CONFIG_ABI_IBCS_ISC */

#ifdef CONFIG_ABI_IBCS_X286
static long linux_to_xnx_signals[NSIGNALS+1] = {
	0,
	IBCS_SIGHUP,	IBCS_SIGINT,	IBCS_SIGQUIT,	IBCS_SIGILL,
	IBCS_SIGTRAP,	IBCS_SIGABRT,	-1,		IBCS_SIGFPE,
	IBCS_SIGKILL,	IBCS_SIGUSR1,	IBCS_SIGSEGV,	IBCS_SIGUSR2,
	IBCS_SIGPIPE,	IBCS_SIGALRM,	IBCS_SIGTERM,	IBCS_SIGSEGV,
	IBCS_SIGCHLD,	IBCS_SIGCONT,	IBCS_SIGSTOP,	IBCS_SIGTSTP,
	IBCS_SIGTTIN,	IBCS_SIGTTOU,	IBCS_SIGUSR1,	IBCS_SIGGXCPU,
	IBCS_SIGGXFSZ,	IBCS_SIGVTALRM,	IBCS_SIGPROF,	IBCS_SIGWINCH,
	20 /*XNX_SIGIO*/, IBCS_SIGPWR,	-1,		-1
};

static long xnx_to_linux_signals[NSIGNALS+1] = {
	0,
	SIGHUP,		SIGINT,		SIGQUIT,	SIGILL,
	SIGTRAP,	SIGIOT,		SIGUNUSED,	SIGFPE,
	SIGKILL,	SIGUNUSED,	SIGSEGV,	SIGUNUSED,
	SIGPIPE,	SIGALRM,	SIGTERM,	SIGUSR1,
	SIGUSR2,	SIGCHLD,	SIGPWR,		SIGPOLL,
	-1,		-1,		-1,		-1,
	-1,		-1,		-1,		-1,
	-1,		-1,		-1,		-1
};
#endif

struct exec_domain ibcs_exec_domain = {
	"iBCS2",
	iBCS_lcall7,
	1 /* PER_SVR4 */, 2 /* PER_SVR3 */,
	ibcs_to_linux_signals,
	linux_to_ibcs_signals,
	svr4_err_map,
	svr4_socktype_map,
	abi_sockopt_map,
	abi_af_map,
	THIS_MODULE,
	NULL
};

#ifdef CONFIG_ABI_IBCS_SCO
struct exec_domain sco_exec_domain = {
	"OpenServer",
	iBCS_lcall7,
	3 /* PER_SCOSVR3 */, 3 /* PER_SCOSVR3 */,
	sco_to_linux_signals,
	linux_to_sco_signals,
	svr4_err_map,
	svr4_socktype_map,
	abi_sockopt_map,
	abi_af_map,
	THIS_MODULE,
	NULL
};
#endif

#ifdef CONFIG_ABI_IBCS_X286
struct exec_domain xnx_exec_domain = {
	"Xenix",
	iBCS_lcall7,
	7 /* PER_XENIX */, 7 /* PER_XENIX */,
	xnx_to_linux_signals,
	linux_to_xnx_signals,
	svr4_err_map,
	svr4_socktype_map,
	abi_sockopt_map,
	abi_af_map,
	THIS_MODULE,
	NULL
};
#endif

#ifdef CONFIG_ABI_IBCS_ISC
struct exec_domain isc_exec_domain = {
	"ISC",
	iBCS_lcall7,
	5 /* PER_ISCR4 */, 5 /* PER_ISCR4 */,
	isc_to_linux_signals,
	linux_to_isc_signals,
	svr4_err_map,
	svr4_socktype_map,
	abi_sockopt_map,
	abi_af_map,
	THIS_MODULE,
	NULL
};
#endif



static void __exit ibcs_cleanup(void)
{
#ifdef CONFIG_ABI_IBCS_ISC
  unregister_exec_domain(&isc_exec_domain);
#endif
#ifdef CONFIG_ABI_IBCS_X286
  unregister_exec_domain(&xnx_exec_domain);
#endif
#ifdef CONFIG_ABI_IBCS_SCO
  unregister_exec_domain(&sco_exec_domain);
#endif
  unregister_exec_domain(&ibcs_exec_domain);
}


static int __init ibcs_init(void)
{
  register_exec_domain(&ibcs_exec_domain);
#ifdef CONFIG_ABI_IBCS_SCO
  register_exec_domain(&sco_exec_domain);
#endif
#ifdef CONFIG_ABI_IBCS_X286
  register_exec_domain(&xnx_exec_domain);
#endif
#ifdef CONFIG_ABI_IBCS_ISC
  register_exec_domain(&isc_exec_domain);
#endif
  return 0;
}

module_init(ibcs_init);
module_exit(ibcs_cleanup);
