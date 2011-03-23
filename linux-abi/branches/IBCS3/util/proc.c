/*
 * Copyright (c) 2008 Christian Lademann, ZLS Software GmbH <cal@zls.de>
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

//#ident "%W% %G%"

#include "../include/util/i386_std.h"
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/file.h>
#if _KSL > 25
#include <linux/fdtable.h>
#endif
#include <linux/string.h>
#include <linux/module.h>
#include <linux/vmalloc.h>
#include <linux/proc_fs.h>
#include <linux/version.h>
#include <asm/uaccess.h>

#include "../include/svr4/types.h"
#include "../include/svr4/stat.h"

#include "../include/util/stat.h"
#include "../include/util/trace.h"

#ifdef CONFIG_PROC_FS

struct proc_dir_entry *proc_abi;
static int abi_gen_proc_write(struct file * file, const char * buf, unsigned long length, void *data);
static int abi_proc_info(char *, char **, off_t, int, int *, void *);

#ifdef CONFIG_ABI_SHINOMAP
#include "../include/util/shinomap.h"
#endif


/*
 * Preliminary /proc/abi - support.
 * Shamelessly copied some parts from scsi_proc.c
 */
static int
abi_gen_proc_write(struct file * file, const char * buf, unsigned long length, void *data) {
	char * buffer;
	int err;

	if (!buf || length>PAGE_SIZE)
		return -EINVAL;

	if (!(buffer = (char *) __get_free_page(GFP_KERNEL)))
		return -ENOMEM;

	if(copy_from_user(buffer, buf, length))
	{
		err =-EFAULT;
		goto out;
	}

	err = -EINVAL;

	if (length < PAGE_SIZE)
		buffer[length] = '\0';
	else if (buffer[PAGE_SIZE-1])
		goto out;

out:
	
	free_page((unsigned long) buffer);
	return err;
}


static int
abi_proc_info(char *buffer, char **start, off_t offset, int length, int *eof, void *data) {
	int len = 0;
	off_t begin = 0;
	off_t pos = 0;

	len += sprintf(buffer + len, "version: %s\n", IBCS_VERSION);
	len += sprintf(buffer + len, "trace_flag: 0x%08x\n", abi_trace_flg);

#ifdef CONFIG_ABI_SHINOMAP
	len += sprintf(buffer + len, "short_inode_mapping: %d\n", short_inode_mapping);
#endif

	pos = begin + len;

	*start = buffer + (offset - begin);	/* Start of wanted data */
	len -= (offset - begin);	/* Start slop */
	if (len > length)
		len = length;	/* Ending slop */
	*eof = (len <= 0 ? 1 : 0);
	return (len);
}


int
abi_proc_init(void)
{
	struct proc_dir_entry *pe;

	/*
	 * This makes /proc/abi and /proc/abi/abi visible.
	 */
	if(! (proc_abi = proc_mkdir("abi", NULL))) {
		printk(KERN_ERR "cannot init /proc/abi\n");
		return -ENOMEM;
	}

	if(! (pe = create_proc_read_entry("abi/abi", S_IFREG | S_IRUGO | S_IWUSR, NULL, abi_proc_info, NULL))) {
		printk(KERN_ERR "cannot init /proc/abi/abi\n");
		remove_proc_entry("abi", NULL);
		return -ENOMEM;
	}

	pe->write_proc = abi_gen_proc_write;

	return 0;
}


void 
abi_proc_exit(void)
{
	/* No, we're not here anymore. Don't show the /proc/abi files. */
	remove_proc_entry("abi/abi", 0);
	remove_proc_entry("abi", 0);
}


#endif /* CONFIG_PROC_FS */
