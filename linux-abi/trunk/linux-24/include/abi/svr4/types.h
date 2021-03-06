/*
 * Copyright (c) 2001 Caldera Deutschland GmbH.
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
#ifndef _ABI_SVR4_TYPES_H
#define _ABI_SVR4_TYPES_H

#ident "%W% %G%"

/*
 * SVR4 type declarations.
 */
#include <linux/highuid.h>
#include <linux/personality.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <abi/util/stat.h>

/*
 * XXX	this will need additions if we support emulation of
 * XXX	64bit SVR4 on 64bit Linux
 */
typedef u_int32_t	svr4_dev_t;
typedef u_int32_t	svr4_ino_t;
typedef u_int32_t	svr4_mode_t;
typedef u_int32_t	svr4_nlink_t;
typedef int32_t		svr4_uid_t;
typedef int32_t		svr4_gid_t;
typedef int32_t		svr4_off_t;
typedef int32_t		svr4_time_t;
typedef struct timeval	svr4_timestruc_t;

typedef int16_t		svr4_o_dev_t;
typedef int16_t		svr4_o_pid_t;
typedef u_int16_t	svr4_o_ino_t;
typedef u_int16_t	svr4_o_mode_t;
typedef int16_t		svr4_o_nlink_t;
typedef u_int16_t	svr4_o_uid_t;
typedef u_int16_t	svr4_o_gid_t;

/*
 * Convert a linux dev number into the SVR4 equivalent.
 */
static __inline svr4_dev_t
linux_to_svr4_dev_t(dev_t dev)
{
	return (dev & 0xff) | ((dev & 0xff00) << 10);
}

/*
 * SVR4 old (=SVR3) dev_t is the same as linux.
 */
static __inline svr4_o_dev_t
linux_to_svr4_o_dev_t(dev_t dev)
{
	return dev;
}

/*
 * If we thought we were in a short inode environment we are
 * probably already too late - getdents() will have likely
 * already assumed short inodes and "fixed" anything with
 * a zero low word (because it must match stat() which must
 * match read() on a directory).
 *
 * We will just have to go along with it.
 */
/*
 * UPDATE: if abi_util was loaded with short_inode_mapping=1,
 * we will use short_inode_map() to return conflict free 
 * 16-bit inode numbers under optimum conditions (less than
 * 65536 different files opened since module load).
 */
static __inline svr4_ino_t
linux_to_svr4_ino_t(ino_t ino)
{
	if (!(current->personality & SHORT_INODE))
		return ino;

	return (svr4_ino_t)short_inode_map(ino);
}

/*
 * Old SVR4 ino_t _must_ be in a short inode enviroment.
 */
static __inline svr4_o_ino_t
linux_to_svr4_o_ino_t(ino_t ino)
{
	return (svr4_ino_t)short_inode_map(ino);
}

/*
 * SVR4 UIDs/GIDs are the same as current Linux ones,
 * old (SVR3) ones are the same as old Linux ones.
 */
static __inline svr4_uid_t
linux_to_svr4_uid_t(uid_t uid)
{
	return uid;
}

static __inline svr4_o_uid_t
linux_to_svr4_o_uid_t(uid_t uid)
{
	return NEW_TO_OLD_UID(uid);
}

static __inline svr4_gid_t
linux_to_svr4_gid_t(gid_t gid)
{
	return gid;
}

static __inline svr4_o_gid_t
linux_to_svr4_o_gid_t(gid_t gid)
{
	return NEW_TO_OLD_GID(gid);
}

#endif /* _ABI_SVR4_TYPES_H */
