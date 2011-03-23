/*
 *   abi/uw7/do_uw7.c - main compilation unit of abi_uw7 module.
 *
 *  provides init/cleanup entry points.
 *  This software is under GPL
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>

#include <abi/abi.h>
#include <abi/uw7.h>
#include <abi/svr4.h>

EXPORT_NO_SYMBOLS;
MODULE_DESCRIPTION("SCO UnixWare 7.x emulator");

extern ABI_func uw7_funcs[];

static void UW7_lcall7(int segment, struct pt_regs * regs)
{
	abi_dispatch(regs, &uw7_funcs[regs->eax & 0xff], 1);
}

struct exec_domain uw7_exec_domain = {
	"UnixWare 7",
	UW7_lcall7,
	14 /* PER_UW7 */, 14 /* PER_UW7 */,
	ibcs_to_linux_signals,
	linux_to_ibcs_signals,
	svr4_err_map,
	svr4_socktype_map,
	abi_sockopt_map,
	abi_af_map,
	THIS_MODULE,
	NULL
};

static int __init init_uw7(void)
{
	printk(KERN_INFO "SCO UnixWare 7.x emulator v0.1\n");
	if (register_exec_domain(&uw7_exec_domain)) {
		printk(KERN_ERR "UW7: Can't register UW7 exec domain (personality=%d)\n",
			PER_UW7);
		return 1;
	}
	if (uw7_proc_init()) {
		unregister_exec_domain(&uw7_exec_domain);
		return 1;
	}
	return 0;
}

static void __exit cleanup_uw7(void)
{
	uw7_proc_cleanup();
	unregister_exec_domain(&uw7_exec_domain);
}

module_init(init_uw7);
module_exit(cleanup_uw7);
