#include <linux/kernel.h>
#include <linux/config.h>
#include <abi/abi.h>

#define SC(name)	(void *)__NR_##name

#ifdef CONFIG_ABI_TRACE
#  define ITR(trace, name, args)	,trace,name,args
#else
#  define ITR(trace, name, args)
#endif

static ABI_func WYSETCP_funcs[] = {
#ifdef CONFIG_ABI_TRACE
   { abi_select,       5       ITR(0, "select",        "dxxxx")},/*  0 */
#else
   { SC(_newselect),    -5      ITR(0, "select",        "dxxxx")},/*  0 */
#endif
   { abi_socket,      Spl     ITR(0, "socket",        "ddd")  }, /*  1 */
   { abi_connect,     Spl     ITR(0, "connect",       "dxd")  }, /*  2 */
   { abi_accept,      Spl     ITR(0, "accept",        "dxx")  }, /*  3 */
   { abi_send,        Spl     ITR(0, "send",          "dxdd")}, /*  4 */
   { abi_recv,        Spl     ITR(0, "recv",          "dxdd")}, /*  5 */
   { abi_bind,        Spl     ITR(0, "bind",          "dxd")  }, /*  6 */
   { abi_setsockopt,  Spl     ITR(0, "setsockopt",    "")     },  /*  7 */
   { abi_listen,      Spl     ITR(0, "listen",        "dd")   }, /*  8 */
   { 0,                 3       ITR(1, "recvmsg",       "dxd")  }, /*  9 */
   { 0,                 3       ITR(1, "sendmsg",       "dxd")  }, /* 10 */
   { abi_getsockopt,  Spl     ITR(0, "getsockopt",    "dddxx")}, /* 11 */
   { abi_recvfrom,    Spl     ITR(0, "recvfrom",      "dxddxd")},/* 12 */
   { abi_sendto,      Spl     ITR(0, "sendto",        "dxddxd")},/* 13 */
   { abi_shutdown,    Spl     ITR(0, "shutdown",      "dd")   }, /* 14 */
   { abi_socketpair,  Spl     ITR(0, "socketpair",    "dddx")},  /* 15 */
   { 0,                 Ukn     ITR(1, "trace",         "")     }, /* 16 */
   { abi_getpeername, Spl     ITR(0, "getpeername",   "dxx")  }, /* 17 */
   { abi_getsockname, Spl     ITR(0, "getsockname",   "")     }, /* 18 */
   { abi_wait3,       1       ITR(0, "wait3",         "x")    }, /* 19 */
};


void iBCS_class_WYSETCP(struct pt_regs *regs) {
	int i;
	
	i = regs->eax >> 8;
	if (i > 19) {
		regs->eax = iABI_errors(-EINVAL);
		regs->eflags |= 1;
		return;
	}

	abi_dispatch(regs, &WYSETCP_funcs[i], 1);
	return;
}
