/*
 *    abi/uw7/proc.c - /proc/abi/uw7 handling for PER_UW7 personality.
 *
 *  This software is under GPL
 */

#include <linux/kernel.h>
#include <linux/stat.h>
#include <linux/proc_fs.h>

#include <abi/abi.h>


#undef DEBUG

#ifdef DEBUG
#define DBG(x...)	printk(x)
#else
#define DBG(x...)
#endif


/* top level /proc/abi/uw7 where we create all our stuff */
static struct proc_dir_entry * uw7;

/* /proc/abi/uw7/proc emulates UW7 /proc */
static struct proc_dir_entry * proc;

/* /proc/abi/uw7/processorfs emulates UW7 /system/processor */
static struct proc_dir_entry * procfs;

/* /system/processor/ctl for controlling the system */
static struct proc_dir_entry * procfs_ctl;

/* /system/processor/<engine> for info on this engine */
static struct proc_dir_entry * procfs_eng[NR_CPUS];

int uw7_proc_init(void)
{
	int i, j;
	char eng[4];

	uw7 = create_proc_entry("uw7", S_IFDIR, abi_proc_entry);
	if (!uw7) {
		printk(KERN_ERR "UW7: Can't create /proc/abi/uw7 entry\n");
		goto out;
	}

	proc = create_proc_entry("proc", S_IFDIR, uw7);
	if (!proc) {
		printk(KERN_ERR "UW7: Can't create /proc/abi/uw7/proc entry\n");
		goto out_uw7;
	}

	procfs = create_proc_entry("processorfs", S_IFDIR, uw7);
	if (!procfs) {
		printk(KERN_ERR "UW7: Can't create /proc/abi/uw7/processorfs entry\n");
		goto out_proc;
	}
	
	procfs_ctl = create_proc_entry("ctl", S_IFREG | S_IWUSR, procfs);
	if (!procfs_ctl) {
		printk(KERN_ERR "UW7: Can't create /proc/abi/uw7/processorfs/ctl entry\n");
		goto out_procfs;
	}

	for (i=0; i<smp_num_cpus; i++) {
		sprintf(eng, "%03d", i);
		procfs_eng[i] = create_proc_entry(eng, S_IFREG|S_IRUGO, procfs);
		if (!procfs_eng[i]) {
			printk(KERN_ERR 
				"UW7: Can't create /proc/abi/uw7/processorfs/%s entry\n", eng);
			goto out_ctl_cpus;
		}
	}

	return 0;

out_ctl_cpus:
	remove_proc_entry("ctl", procfs);
	for (j=0; j<i; j++) {
		sprintf(eng, "%03d", j);
		remove_proc_entry(eng, procfs);
	}
	
out_procfs:
	remove_proc_entry("processorfs", uw7);

out_proc:
	remove_proc_entry("proc", uw7);

out_uw7:
	remove_proc_entry("uw7", abi_proc_entry);

out:
	return 1;

}

void uw7_proc_cleanup(void)
{
	int i;
	char eng[4];

	for (i=0; i<smp_num_cpus; i++) {
		sprintf(eng, "%03d", i);
		remove_proc_entry(eng, procfs);
	}
	remove_proc_entry("ctl", procfs);
	remove_proc_entry("processorfs", proc);
	remove_proc_entry("proc", proc);
	remove_proc_entry("uw7", abi_proc_entry);
}
