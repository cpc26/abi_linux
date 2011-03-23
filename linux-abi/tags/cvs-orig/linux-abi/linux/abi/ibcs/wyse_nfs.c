#include <linux/kernel.h>
#include <linux/config.h>
#include <abi/abi.h>

#define SC(name)	(void *)__NR_##name

#ifdef CONFIG_ABI_TRACE
#  define ITR(trace, name, args)	,trace,name,args
#else
#  define ITR(trace, name, args)
#endif

static ABI_func WYSENFS_funcs[] = {
   { 0,			Ukn	ITR(1, "nfs_svc",	"")	}, /*  0 */
   { 0,			Ukn	ITR(1, "async_daemon",	"")	}, /*  1 */
   { 0,			Ukn	ITR(1, "nfs_getfh",	"")	}, /*  2 */
   { 0,			Ukn	ITR(1, "nfsmount",	"")	} /*  3 */
};


void iBCS_class_WYSENFS(struct pt_regs *regs) {
	int i;
	
	i = regs->eax >> 8;
	if (i > 3) {
		regs->eax = iABI_errors(-EINVAL);
		regs->eflags |= 1;
		return;
	}		

	abi_dispatch(regs, &WYSENFS_funcs[i],1);
	return;
}
