/*
 *   abi/uw7/mmap.c - mmap(2) system call.
 *
 *  This software is under GPL
 */

#include <linux/mm.h>
#include <linux/errno.h>
#include <linux/file.h>
#include <linux/mman.h>
#include <linux/smp_lock.h>

#include <asm/uaccess.h>

#include <abi/abi.h>
#include <abi/uw7.h>

int uw7_mmap(unsigned long addr, unsigned long len, unsigned long prot, unsigned long flags, 
	unsigned long fd, unsigned long offset)
{
	int error = -EBADF;
	struct file * file = NULL;

	down(&current->mm->mmap_sem);
	lock_kernel();
	if (flags & UW7_MAP_ANONYMOUS) {
		flags |= MAP_ANONYMOUS;
		flags &= ~UW7_MAP_ANONYMOUS;
	}
	if (!(flags & MAP_ANONYMOUS))
		if (!(file = fget(fd)))
			goto out;
	flags &= ~(MAP_EXECUTABLE | MAP_DENYWRITE);
	error = do_mmap(file, addr, len, prot, flags, offset);
	printk(KERN_ERR 
		"mmap(addr=%08lx, len=%08lx, prot=%08lx, flags=%08lx, off=%08lx)=%08lx for <%s>\n",
		addr, len, prot, flags, offset, error, current->comm);
out:
	if (file)
		fput(file);
	unlock_kernel();
	up(&current->mm->mmap_sem);
	return error;
}
