/*
 *  abi/abi_common/proc.c - /proc/abi handling.
 *
 *  Exports a symbol abi_proc_entry to modules so they can create
 *  subdirectories in /proc/abi for their respective personalities.
 *
 */

#include <linux/config.h>
#define __NO_VERSION__
#include <linux/module.h>
#include <linux/version.h>

#include <linux/sched.h>
#include <linux/proc_fs.h>

#ifndef CONFIG_PROC_FS
#error Support for /proc is required. Please reconfigure.
#endif

struct proc_dir_entry * abi_proc_entry;
EXPORT_SYMBOL(abi_proc_entry);

int abi_proc_init(void)
{
	abi_proc_entry = create_proc_entry("abi", S_IFDIR, NULL);
	if (!abi_proc_entry) {
		printk(KERN_ERR "ABI: Can't create /proc/abi\n");
		return 1;
	}
	return 0;
}

void abi_proc_cleanup(void)
{
	remove_proc_entry("abi", NULL);
}
