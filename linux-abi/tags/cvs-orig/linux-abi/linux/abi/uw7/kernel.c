/*
 *   abi/uw7/kernel.c - various UW7 system calls.
 *
 *  This software is under GPL
 */

#include <linux/errno.h>
#include <linux/sched.h>

#include <asm/uaccess.h>

#include <abi/abi.h>
#include <abi/uw7.h>


int uw7_sleep(int seconds)
{
	struct timespec t;
	mm_segment_t old_fs;
	int error;

	t.tv_sec = seconds;
	t.tv_nsec = 0;
	old_fs = get_fs();
	set_fs(get_ds());
	error = SYS(nanosleep)(&t, NULL);
	set_fs(old_fs);
	return error;
}

#define UW7_MAXUID      60002

int uw7_seteuid(int uid)
{
	if (uid < 0 || uid > UW7_MAXUID)
		return -EINVAL;
	return SYS(setreuid)(-1, uid);
}

int uw7_setegid(int gid)
{
	if (gid < 0 || gid > UW7_MAXUID)
		return -EINVAL;
	return SYS(setreuid)(-1, gid);
}

int uw7_lseek64(unsigned int fd, unsigned int off, unsigned int off_hi, unsigned int orig)
{
	/* if (off_hi != 0) (commented because some buggy progs set off_hi = -1)
		return -EINVAL; */
	return SYS(lseek)(fd, (off_t)off, orig);
}

/* can't call sys_pread() directly because off is 32bit on UW7 */
int uw7_pread(unsigned int fd, char * buf, int count, long off)
{
	return SYS(pread)(fd, buf, count, (loff_t)off);
}

int uw7_pread64(unsigned int fd, char * buf, int count, unsigned int off, unsigned int off_hi)
{
	if (off_hi != 0)
		return -EINVAL;

	return SYS(pread)(fd, buf, count, (loff_t)off);
}

int uw7_pwrite64(unsigned int fd, char * buf, int count, unsigned int off, unsigned int off_hi)
{
	if (off_hi != 0)
		return -EINVAL;

	return SYS(pread)(fd, buf, count, (loff_t)off);
}

/* can't call sys_pwrite() directly because off is 32bit on UW7 */
int uw7_pwrite(unsigned int fd, char * buf, int count, long off)
{
	return SYS(pwrite)(fd, buf, count, (loff_t)off);
}

int uw7_stty(int fd, int cmd)
{
	return -EIO;
}

int uw7_gtty(int fd, int cmd)
{
	return -EIO;
}
