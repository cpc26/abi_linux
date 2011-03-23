/*
 *  linux/abi/bsd/bsdioctl.c
 *
 *  Copyright (C) 1994  Mike Jagdis
 *
 * $Id$
 * $Source$
 */

#include <linux/config.h>


#include <linux/version.h>

#include <asm/uaccess.h>

#include <linux/errno.h>
#include <linux/stat.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/sys.h>
#include <linux/termios.h>
#include <linux/time.h>
#include <linux/mm.h>

#include <abi/abi.h>
#include <abi/bsd.h>

#ifdef CONFIG_ABI_TRACE
#include <abi/trace.h>
#endif



static inline char *fix(int n) {
        static char char_class[4];
        
         char_class[0] = n & 0xFF0000 ? (char)((n >> 16) & 0xFF) : '.';
         char_class[1] = n & 0x00FF00 ? (char)((n >>  8) & 0xFF) : '.';
         char_class[2] = n & 0x0000FF ? (char)((n      ) & 0xFF) : '.';
         char_class[3] = 0;

        return char_class;
}

extern int bsd_ioctl_termios(int fd, unsigned int func, void *arg);
extern int abi_ioctl_video(int fd, unsigned int ioctl_num, void *arg);
extern int abi_ioctl_console(int fd, unsigned int ioctl_num, void *arg);
extern int abi_ioctl_socksys(int fd, unsigned int ioctl_num, void *arg);

/* Some of these are used by SVR3/4 too... */
static int bsd_ioctl_file(int fd, unsigned int func, void *arg)
{
	switch (func) {
		case BSD__IOV('f', 1): case BSD__IO('f', 1): /* FIOCLEX */
			FD_SET(fd, &current->files->close_on_exec);
			return 0;

		case BSD__IOV('f', 2): case BSD__IO('f', 2): /* FIONCLEX */
			FD_CLR(fd, &current->files->close_on_exec);
			return 0;

		case BSD__IOV('f', 3): case BSD__IO('f', 3): { /* FIORDCHK */
			int error, nbytes;
			mm_segment_t old_fs;

			old_fs = get_fs();
			set_fs (get_ds());
			error = SYS(ioctl)(fd, FIONREAD, &nbytes);
			set_fs(old_fs);

			return (error <= 0 ? error : nbytes);
		}

		case BSD__IOW('f', 123, int): /* FGETOWN */
			return SYS(ioctl)(fd, FIOGETOWN, arg);

		case BSD__IOW('f', 124, int): /* FSETOWN */
			return SYS(ioctl)(fd, FIOSETOWN, arg);

		case BSD__IOW('f', 125, int): /* FIOASYNC */
			return SYS(ioctl)(fd, FIOASYNC, arg);

		case BSD__IOW('f', 126, int): /* FIONBIO */
			return SYS(ioctl)(fd, FIONBIO, arg);

		case BSD__IOR('f', 127, int): /* FIONREAD */
			return SYS(ioctl)(fd, FIONREAD, arg);
	}

	printk(KERN_ERR "iBCS: file ioctl 0x%08lx unsupported\n",
		(unsigned long)func);
	return -EINVAL;
}


static int
do_bsd_ioctl(struct pt_regs *regs, int fd, unsigned long ioctl_num, void *arg)
{

	unsigned int class = ioctl_num >> 8;

	switch (class) {
		case 't':
			return bsd_ioctl_termios(fd, ioctl_num, arg);

		case 'f':
			return bsd_ioctl_file(fd, ioctl_num, arg);

		case 'C':
		case 'c':
			return abi_ioctl_console(fd, ioctl_num, arg);

		case ('i' << 16) | ('C' << 8):	/* iBCS2 POSIX */
			return abi_ioctl_video(fd, ioctl_num & 0xFF, arg);

	}

	/* If we haven't handled it yet it must be a BSD style ioctl
	 * with a (possible) argument description in the high word of
	 * the opcode.
	 */
	switch (class & 0xff) {
		/* From SVR4 as specified in sys/iocomm.h */
		case 'f':
			return bsd_ioctl_file(fd, ioctl_num, arg);

		/* BSD or V7 terminal ioctls. */
		case 't':
			return bsd_ioctl_termios(fd, ioctl_num, arg);

		/* "Traditional" BSD and Wyse V/386 3.2.1A TCP/IP ioctls. */
		case 's':
		case 'r':
		case 'i':
			return abi_ioctl_socksys(fd, ioctl_num, arg);

		/* SVR3 streams based socket TCP/IP ioctls.
		 * These are handed over to the standard ioctl
		 * handler since /dev/socksys is an emulated device
		 * and front ends any sockets created through it.
		 * Note that 'S' ioctls without the BSDish argument
		 * type in the high bytes are STREAMS ioctls and 'I'
		 * ioctls without the BSDish type in the high bytes
		 * are the STREAMS socket module ioctls. (see above).
		 */
		case 'S':
		case 'R':
		case 'I':
			return abi_ioctl_socksys(fd, ioctl_num, arg);
	}

	/* If nothing has handled it yet someone may have to do some
	 * more work...
	 */
	printk(KERN_ERR "iBCS: ioctl(%d, %lx[%s], 0x%lx) unsupported\n",
		fd, ioctl_num, fix(class), (unsigned long)arg);

	return -EINVAL;
}

int
bsd_ioctl(struct pt_regs *regs)
{
	int fd;
	unsigned int ioctl_num;
	void *arg;

	fd = (int)get_syscall_parameter (regs, 0);
	ioctl_num = (unsigned int)get_syscall_parameter (regs, 1);
	arg = (void *)get_syscall_parameter (regs, 2);
	return do_bsd_ioctl(regs, fd, ioctl_num, arg);
}
