#include <linux/kernel.h>
#include <linux/config.h>
#define __NO_VERSION__
#include <linux/module.h>
#include <abi/abi.h>
#include <abi/abi4.h>
#include <asm/uaccess.h>
#define SC(name)	(void *)__NR_##name

#ifdef CONFIG_ABI_TRACE
#  define ITR(trace, name, args)	,trace,name,args
#else
#  define ITR(trace, name, args)
#endif

static ABI_func ISC_funcs[] = {
   { 0,			Ukn	ITR(1, "isc_sysisc0",	"")	}, /* 00 */
   { isc_setostype,	1	ITR(0, "isc_setostype",	"d")	}, /* 01 */
   { SC(rename),	-2	ITR(0, "isc_rename",	"ss")	}, /* 02 */
   { abi_sigaction,	3	ITR(0, "isc_sigaction",	"dxx")	}, /* 03 */
   { abi_sigprocmask,	3	ITR(0, "isc_sicprocmask","dxx")	}, /* 04 */
   { 0,			1	ITR(0, "isc_sigpending","x")	}, /* 05 */
   { SC(getgroups),	-2	ITR(0, "isc_getgroups",	"dp")	}, /* 06 */
   { SC(setgroups),	-2	ITR(0, "isc_setgroups",	"dp")	}, /* 07 */
   { 0,			Ukn	ITR(1, "pathconf",	"")	}, /* 08 */
   { 0,			Ukn	ITR(1, "fpathconf",	"")	}, /* 09 */
   { ibcs_sysconf,	1	ITR(0, "sysconf",	"d")	}, /* 10 */
   { SC(waitpid),	-3	ITR(0, "isc_waitpid",	"dxx")	}, /* 11 */
   { SC(setsid),	-ZERO	ITR(0, "isc_setsid",	"")	}, /* 12 */
   { SC(setpgid),	-2	ITR(0, "isc_setpgid",	"dd")	}, /* 13 */
   { 0,			Ukn	ITR(1, "isc_adduser",	"")	}, /* 14 */
   { 0,			Ukn	ITR(1, "isc_setuser",	"")	}, /* 15 */
   { 0,			Ukn	ITR(1, "isc_sysisc16",	"")	}, /* 16 */
   { abi_sigsuspend,	Spl	ITR(0, "isc_sigsuspend","x")	}, /* 17 */
   { SC(symlink),	-2	ITR(0, "isc_symlink",	"ss")	}, /* 18 */
   { SC(readlink),	-3	ITR(0, "isc_readlink",	"spd")	}, /* 19 */
   { 0,			Ukn	ITR(1, "isc_getmajor",	"")	} /* 20 */
};


void iBCS_class_ISC(struct pt_regs *regs) {
	int i;

	get_user(i, ((unsigned long *)regs->esp)+1);
	if (i > 20) {
		regs->eax = iABI_errors(-EINVAL);
		regs->eflags |= 1;
		return;
	}
	
	abi_dispatch(regs, &ISC_funcs[i], 2);
	return;
}
