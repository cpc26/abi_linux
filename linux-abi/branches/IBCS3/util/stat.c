/*
 * Mostly ripped from Al Viro's stat-a-AC9-10 patch, 2001 Christoph Hellwig.
 */

//#ident "%W% %G%"

#include <linux/module.h>		/* needed to shut up modprobe */
#include <linux/moduleparam.h>
#include <linux/fs.h>
#include <linux/stat.h>
#include <linux/sched.h>
#include <linux/file.h>
#include <linux/namei.h>

#include "../include/util/revalidate.h"
#include "../include/util/stat.h"

#ifdef CONFIG_ABI_PROC
#include "../include/util/proc.h"
#endif

#ifdef CONFIG_ABI_SHINOMAP
#include "../include/util/shinomap.h"
#endif


MODULE_DESCRIPTION("Linux-ABI helper routines");
MODULE_AUTHOR("Christoph Hellwig, ripped from kernel sources/patches");
MODULE_LICENSE("GPL");
MODULE_INFO(supported,"yes");
MODULE_INFO(bugreport,"agon04@users.sourceforge.net");


static int __init
util_module_init(void)
{
#if defined(CONFIG_PROC_FS) && defined(CONFIG_ABI_PROC)
	if(abi_proc_init())
		return -ENOMEM;
#endif

#ifdef CONFIG_ABI_SHINOMAP
	if(abi_shinomap_init())
		return -ENOMEM;
#endif

	return 0;
}



static void __exit
util_module_exit(void)
{
#ifdef CONFIG_ABI_SHINOMAP
	abi_shinomap_exit();
#endif

#if defined(CONFIG_PROC_FS) && defined(CONFIG_ABI_PROC)
	abi_proc_exit();
#endif
}


module_init(util_module_init);
module_exit(util_module_exit);
