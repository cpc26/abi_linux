#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>

#include <abi/abi.h>
#include <abi/abi4.h>
#include <abi/svr4.h>
#include <abi/solaris.h>

EXPORT_NO_SYMBOLS;

extern ABI_func  svr4_generic_funcs[];

static ABI_func Solaris_funcs[] = {
   { 0,			Ukn	ITR(1, "vtrace",       	"")	}, /*   142 */
   { 0,			Ukn	ITR(1, "fork1",		"")	}, /*   143 */
   { 0,			Ukn	ITR(1, "sigtimedwait",	"")	}, /*	144 */
   { 0,			Ukn	ITR(1, "lwp_info",	"")	}, /*	145 */
   { 0,			Ukn	ITR(1, "yield",		"")	}, /*	146 */
   { 0,			Ukn	ITR(1, "lwp_sema_wait",	"")	}, /*	147 */
   { 0,			Ukn	ITR(1, "lwp_sema_post",	"")	}, /*	148 */
   { 0,			Ukn	ITR(1, "lwp_sema_trywait","")	}, /*	149 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /*	150 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /*	151 */
   { 0,			Ukn	ITR(1, "modctl",       	"")	}, /*	152 */
   { 0,			Ukn	ITR(1, "fchroot",	"")	}, /*	153 */
   { 0,			Ukn	ITR(1, "utimes",	"")	}, /*	154 */
   { 0,			Ukn	ITR(1, "vhangup",      	"")	}, /*	155 */
   { SC(gettimeofday),	-2	ITR(0, "gettimeofday",	"xx")	}, /*	156 */
   { SC(getitimer),	-2	ITR(0, "getitimer",    	"dx")	}, /*	157 */
   { SC(setitimer),	-3	ITR(0, "setitimer",	"dxx")	}, /*	158 */
   { 0,			Ukn	ITR(1, "lwp_create",	"")	}, /*	159 */
   { 0,			Ukn	ITR(1, "lwp_exit",	"")	}, /*	160 */
   { 0,			Ukn	ITR(1, "lwp_suspend",	"")	}, /*	161 */
   { 0,			Ukn	ITR(1, "lwp_continue",	"")	}, /*	162 */
   { 0,			Ukn	ITR(1, "lwp_kill",	"")	}, /*	163 */
   { 0,			Ukn	ITR(1, "lwp_self",	"")	}, /*	164 */
   { 0,			Ukn	ITR(1, "lwp_setprivate","")	}, /*	165 */
   { 0,			Ukn	ITR(1, "lwp_getprivate","")	}, /*	166 */
   { 0,			Ukn	ITR(1, "lwp_wait",	"")	}, /*	167 */
   { 0,			Ukn	ITR(1, "lwp_mutex_unlock","")	}, /*	168 */
   { 0,			Ukn	ITR(1, "lwp_mutex_lock","")	}, /*	169 */
   { 0,			Ukn	ITR(1, "lwp_cond_wait",	"")	}, /*	170 */
   { 0,			Ukn	ITR(1, "lwp_cond_signal","")	}, /*	171 */
   { 0,			Ukn	ITR(1, "lwp_cond_broadcast","")	}, /*	172 */
   { SC(pread),	       	-4	ITR(1, "pread",		"dpdd")	}, /*	173 */
   { SC(pwrite),	-4	ITR(1, "pwrite",	"dpdd")	}, /*	174 */
   { sol_llseek,      	Spl	ITR(1, "llseek",	"dxxd")	}, /*	175 */
   { 0,			Ukn	ITR(1, "inst_sync",	"")	}, /*	176 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /*	177 */
   { 0,			Ukn	ITR(1, "kaio",		"")	}, /*	178 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /*	179 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /*	180 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /*	181 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /*	182 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /*	183 */
   { 0,			Ukn	ITR(1, "tsolsys",      	"")	}, /*	184 */
   { sol_acl,		4	ITR(1, "acl",		"sddp")	}, /*	185 */
   { 0,			Ukn	ITR(1, "auditsys",	"")	}, /*	186 */
   { 0,			Ukn	ITR(1, "processor_bind","")	}, /*	187 */
   { 0,			Ukn	ITR(1, "processor_info","")	}, /*	188 */
   { 0,			Ukn	ITR(1, "p_online",     	"")	}, /*	189 */
   { 0,			Ukn	ITR(1, "sigqueue",	"")	}, /*	190 */
   { 0,			Ukn	ITR(1, "clock_gettime",	"")	}, /*	191 */
   { 0,			Ukn	ITR(1, "clock_settime",	"")	}, /*	192 */
   { 0,			Ukn	ITR(1, "clock_getres",	"")	}, /*	193 */
   { 0,			Ukn	ITR(1, "timer_create",	"")	}, /*	194 */
   { 0,			Ukn	ITR(1, "timer_delete",	"")	}, /*	195 */
   { 0,			Ukn	ITR(1, "timer_settime",	"")	}, /*	196 */
   { 0,			Ukn	ITR(1, "timer_gettime",	"")	}, /*	197 */
   { 0,			Ukn	ITR(1, "timer_getoverrun","")	}, /*	198 */
   { SC(nanosleep),	-2	ITR(1, "nanosleep",	"pp")	}, /*	199 */
   { 0,			Ukn	ITR(1, "modstat",	"")	}, /*	200 */
   { 0,			Ukn	ITR(1, "facl",		"")	}, /*	201 */
   { SC(setreuid),	-2	ITR(1, "setreuid",	"dd")	}, /*	202 */
   { SC(setregid),	-2	ITR(1, "setregid",	"dd")	}, /*	203 */
   { 0,			Ukn	ITR(1, "install_utrap",	"")	}, /*	204 */
   { 0,			Ukn	ITR(1, "signotify",	"")	}, /*	205 */
   { 0,			Ukn	ITR(1, "schedctl",	"")	}, /*	206 */
   { 0,			Ukn	ITR(1, "pset",		"")	}, /*	207 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /* 208 */
   { 0,			Ukn	ITR(1, "resolvepath",	"")	}, /* 209 */
   { 0,			Ukn	ITR(1, "signotifywait",	"")	}, /* 210 */
   { 0,			Ukn	ITR(1, "lwp_sigredirect","")	}, /* 211 */
   { 0,			Ukn	ITR(1, "lwp_alarm",	"")	}, /* 212 */
   { sol_getdents64,	3  	ITR(0, "getdents64", 	"dxd")	}, /* 213 */
   { sol_mmap64,	7	ITR(1, "mmap64",      "pxdddxx")}, /*214 */
   { sol_stat64,      	2	ITR(0, "stat64",       	"sp")	}, /* 215 */
   { sol_lstat64,	2	ITR(0, "lstat64",	"sp")	}, /* 216 */
   { sol_fstat64,	2	ITR(0, "fstat64",       "dp")	}, /* 217 */
   { 0,			Ukn	ITR(1, "statvfs64",	"")	}, /* 218 */
   { 0,			Ukn	ITR(1, "fstatvfs64",	"")	}, /* 219 */
   { 0,			Ukn	ITR(1, "setrlimit64",	"")	}, /* 220 */
   { 0,			Ukn	ITR(1, "getrlimit64",	"")	}, /* 221 */
   { 0,			Ukn	ITR(1, "pread64",	"")	}, /* 222 */
   { 0,			Ukn	ITR(1, "pwrite64",	"")	}, /* 223 */
   { 0,			Ukn	ITR(1, "creat64",	"")	}, /* 224 */
   { sol_open64,      	3	ITR(0, "open64",       	"soo")	}, /* 225 */
   { 0,			Ukn	ITR(1, "rpcsys",	"")	}, /* 226 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /* 227 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /* 228 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /* 229 */
   { abi_socket,	Spl	ITR(1, "so_socket",	"ddd")	}, /* 230 */
   { abi_socketpair,	Spl	ITR(1, "so_socketpair",	"dddx")	}, /* 231 */
   { abi_bind,	Spl	ITR(1, "bind",		"dxd")	}, /* 232 */
   { abi_listen,	Spl	ITR(1, "listen",	"dd")	}, /* 233 */
   { abi_accept,	Spl	ITR(1, "accept",	"dxx")	}, /* 234 */
   { abi_connect,	Spl	ITR(1, "connect",	"dxd")	}, /* 235 */
   { abi_shutdown,	Spl	ITR(1, "shutdown",	"dd")	}, /* 236 */
   { abi_recv,	Spl	ITR(1, "recv",		"dxdd")	}, /* 237 */
   { abi_recvfrom,	Spl	ITR(1, "recvfrom",     "dxddxd")}, /* 238 */
   { 0,			Ukn	ITR(1, "recvmsg",	"")	}, /* 239 */
   { abi_send,	Spl	ITR(1, "send",		"dxdd")	}, /* 240 */
   { 0,     	 	Ukn	ITR(0, "sendmsg",      	"")	}, /* 241 */
   { abi_sendto,	Spl	ITR(1, "sendto",       "dxddxd")}, /* 242 */
   { abi_getpeername,	Spl	ITR(1, "getpeername",	"dxx")	}, /* 243 */
   { abi_getsockname,	Spl	ITR(1, "getsockname",	"")	}, /* 244 */
   { abi_getsockopt,	Spl	ITR(1, "getsockopt",	"dddxx")}, /* 245 */
   { abi_setsockopt,	Spl	ITR(1, "setsockopt",	"dddxd")}, /* 246 */
   { 0,			Ukn	ITR(1, "sockconfig",	"")	}, /* 247 */
   { 0,			Ukn	ITR(1, "ntp_gettime",	"")	}, /* 248 */
   { 0,     	 	Ukn	ITR(0, "ntp_adjtime",  	"")	}, /* 249 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /* 250 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /* 251 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /* 252 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /* 253 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /* 254 */
   { 0,			Ukn	ITR(1, "?",		"")	}  /* 255 */
};



static void Solaris_lcall7(int segment, struct pt_regs * regs)
{
	int i = regs->eax & 0xff;
	ABI_func *p;

	if (i < 142)
		p = &svr4_generic_funcs[i];
	else
		p = &Solaris_funcs[i - 142];

	abi_dispatch(regs, p, 1);
}

extern struct map_segment svr4_err_map[];
extern struct map_segment svr4_socktype_map[];
extern struct map_segment abi_sockopt_map[];
extern struct map_segment abi_af_map[];

extern long linux_to_ibcs_signals[];
extern long ibcs_to_linux_signals[];


struct exec_domain solaris_exec_domain = {
	"Solaris/x86",
	Solaris_lcall7,
	13 /* PER_SOLARIS */, 13 /* PER_SOLARIS */,
	ibcs_to_linux_signals,
	linux_to_ibcs_signals,
	svr4_err_map,
	svr4_socktype_map,
	abi_sockopt_map,
	abi_af_map,
	THIS_MODULE,
	NULL
};


static void __exit solaris_cleanup(void)
{
	unregister_exec_domain(&solaris_exec_domain);
}

static int __init solaris_init(void)
{
	return register_exec_domain(&solaris_exec_domain);
}

module_init(solaris_init);
module_exit(solaris_cleanup);
