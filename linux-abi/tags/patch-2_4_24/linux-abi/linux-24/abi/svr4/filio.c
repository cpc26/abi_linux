/*
 * Copyright (c) 2001 Christoph Hellwig.
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

#ident "%W% %G%"

/*
 * SVR4 file ioctls.
 */

#include <linux/sched.h>
#include <linux/file.h>
#include <linux/syscall.h>
#include <asm/uaccess.h>

#include <abi/ioctl.h>


int
svr4_fil_ioctl(int fd, u_int cmd, caddr_t data)
{
	switch (cmd) {
	/* FIOCLEX */
	case BSD__IOV('f', 1):
	case BSD__IO('f', 1):
		FD_SET(fd, current->files->close_on_exec);
		return 0;

	/* FIONCLEX */
	case BSD__IOV('f', 2):
	case BSD__IO('f', 2):
		FD_CLR(fd, current->files->close_on_exec);
		return 0;

	case BSD__IOV('f', 3):
	case BSD__IO('f', 3): {
		int		error, nbytes;
		mm_segment_t	fs;

		fs = get_fs();
		set_fs(get_ds());
		error = sys_ioctl(fd, FIONREAD, &nbytes);
		set_fs(fs);
		
		return (error <= 0 ? error : nbytes);
	}

	/* FGETOWN */
	case BSD__IOW('f', 123, int):
		return sys_ioctl(fd, FIOGETOWN, data);

	/* FSETOWN */
	case BSD__IOW('f', 124, int):
		return sys_ioctl(fd, FIOSETOWN, data);

	/* FIOASYNC */
	case BSD__IOW('f', 125, int):
		return sys_ioctl(fd, FIOASYNC, data);

	/* FIONBIO */
	case BSD__IOW('f', 126, int):
		return sys_ioctl(fd, FIONBIO, data);

	/* FIONREAD */
	case BSD__IOR('f', 127, int):
		return sys_ioctl(fd, FIONREAD, data);
	}

	printk(KERN_ERR __FUNCTION__ ": file ioctl 0x%08x unsupported\n", cmd);
	return -EINVAL;
}
