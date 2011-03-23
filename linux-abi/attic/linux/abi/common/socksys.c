/*
 *  linux/ibcs/socksys.h
 *
 *  Copyright 1994-1996  Mike Jagdis (jaggy@purplet.demon.co.uk)
 *
 * $Id$
 * $Source$
 */

#include <linux/config.h>
#include <linux/module.h>

#include <linux/version.h>

#include <linux/types.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/major.h>
#include <linux/kernel.h>
#include <linux/in.h>
#include <linux/net.h>
#include <linux/sched.h>
#include <linux/signal.h>
#include <linux/socket.h>
#include <net/sock.h>
#include <linux/malloc.h>
#include <linux/mm.h>
#include <linux/un.h>
#include <linux/utsname.h>
#include <linux/time.h>
#include <linux/termios.h>
#include <linux/sys.h>
#include <linux/file.h>
#include <linux/poll.h>
#include <linux/smp_lock.h>
#include <linux/capability.h>
#include <linux/init.h>

#include <asm/uaccess.h>

#include <abi/abi.h>
#include <abi/map.h>
#include <abi/compat.h>

#ifdef CONFIG_ABI_TRACE
#include <abi/trace.h>
#endif

#include <abi/socksys.h>
#include <abi/tli.h>

/* I believe /proc should be mandatory */
#ifndef CONFIG_PROC_FS
#error You really need /proc support for ABI. Please reconfigure!
#endif

static int socksys_major;


static int socksys_read(struct file *filep,
			char *buf, size_t count, loff_t *offset);
static int socksys_write(struct file *filep,
			const char *buf, size_t count, loff_t *offset);
static int socksys_open(struct inode *ino, struct file *filep);
static int socksys_close(struct inode *ino, struct file *filep);
static unsigned int socksys_poll(struct file *filep, struct poll_table_struct *wait);


/* socksys_fops defines the file operations that can be applied to the
 * /dev/socksys device.
 */
static struct file_operations socksys_fops = {
	NULL,			/* llseek */
	socksys_read,		/* read */
	socksys_write,		/* write */
	NULL,			/* readdir */
	NULL,			/* poll */
	NULL,			/* ioctl - ibcs_ioctl calls ibcs_ioctl_socksys*/
	NULL,			/* mmap */
	socksys_open,		/* open */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,1,119)
	NULL,			/* flush */
#endif
	socksys_close,		/* release */
	NULL,			/* fsync */
	NULL,			/* fasync */
	NULL,			/* check_media_change */
	NULL,			/* revalidate */
	NULL			/* lock */
};

/* socksys_socket_fops defines the file operations that can be applied to
 * sockets themselves. This gets initialised when the first socket is
 * created.
 */
static struct file_operations socksys_socket_fops = {
	NULL,			/* llseek */
	NULL,			/* read */
	NULL,			/* write */
	NULL,			/* readdir */
	NULL,			/* poll */
	NULL,			/* ioctl */
	NULL,			/* mmap */
	NULL,			/* open */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,1,119)
	NULL,			/* flush */
#endif
	NULL,			/* release */
	NULL,			/* fsync */
	NULL,			/* fasync */
	NULL,			/* check_media_change */
	NULL,			/* revalidate */
	NULL			/* lock */
};
static int (*sock_close)(struct inode *inode, struct file *file);
static unsigned int (*sock_poll)(struct file *filep, struct poll_table_struct *wait);


void
inherit_socksys_funcs(unsigned int fd, int state)
{
#ifdef CONFIG_ABI_XTI
	struct T_private *priv;
#endif
	struct inode *ino;
	struct file *filep = fget(fd);
	if (!filep)
		return;

	ino = filep->f_dentry->d_inode;

#ifdef SO_BSDCOMPAT
	/* SYSV sockets are BSD like with respect to ICMP errors
	 * with UDP rather than RFC conforming. I think.
	 */
	ino->u.socket_i.sk->bsdism = 1;
#endif

#ifdef CONFIG_ABI_XTI
	priv = (struct T_private *)kmalloc(sizeof(struct T_private), GFP_KERNEL);
	if (priv) {
		priv->magic = XTI_MAGIC;
		priv->state = state;
		priv->offset = 0;
		priv->pfirst = priv->plast = NULL;
	}
	filep->private_data = priv;
#endif

	/* If our file operations don't appear to match
	 * what the socket system is advertising it may
	 * be because we haven't initialised ours at all
	 * yet or it may be because the old socket system
	 * module was unloaded and reloaded. This isn't
	 * entirely safe because we may still have open
	 * sockets which *should* use the old routines
	 * until they close - tough, for now.
	 */
	if (socksys_socket_fops.read != filep->f_op->read) {
		memcpy(&socksys_socket_fops,
			filep->f_op,
			sizeof(struct file_operations));
		sock_close = socksys_socket_fops.release;
		sock_poll = socksys_socket_fops.poll;
		socksys_socket_fops.release = socksys_close;
		socksys_socket_fops.poll = socksys_poll;
	}
	filep->f_op = &socksys_socket_fops;
#if 1
	ino->i_mode = 0020000; /* S_IFCHR */
	ino->i_rdev = MKDEV(socksys_major, 0);
#endif
	fput(filep);
	//	MOD_INC_USE_COUNT;
}

EXPORT_SYMBOL(inherit_socksys_funcs);

static int
spx_connect(unsigned int fd, int spxnum)
{
	int newfd, err, args[3];
	struct sockaddr_un addr = {
		AF_UNIX, "/tmp/.X11-unix/X0"
	};
	mm_segment_t old_fs = get_fs();

#ifdef CONFIG_ABI_TRACE
	if (ibcs_trace & TRACE_SOCKSYS)
		printk(KERN_DEBUG "%d iBCS: SPX: %u choose service %d\n",
			current->pid, fd, spxnum);
#endif

	/* Rather than use an explicit path to the X :0 server
	 * socket we should use the given number to look up a path
	 * name to use (we can't rely on servers registering their
	 * sockets either - for one thing we don't emulate that yet
	 * and for another thing different OS binaries do things in
	 * different ways but all must interoperate).
	 * I suggest putting the mapping in, say, /dev/spx.map/%d
	 * where each file is a symlink containing the path of the
	 * socket to use. Then we can just do a readlink() here to
	 * get the pathname.
	 *   Hey, this is what we do here now!
	 */
	addr.sun_family = AF_UNIX;
	sprintf(addr.sun_path, "/dev/spx.map/%u", spxnum);
	set_fs(get_ds());
	err = SYS(readlink)(addr.sun_path, addr.sun_path,
				sizeof(addr.sun_path)-1);
	set_fs(old_fs);
	if (err == -ENOENT) {
#ifdef CONFIG_ABI_TRACE
		if (ibcs_trace & TRACE_SOCKSYS)
			printk(KERN_DEBUG
				"%d iBCS: SPX: %u no symlink \"%s\", try X :0\n",
				current->pid, fd, addr.sun_path);
#endif
		strcpy(addr.sun_path, "/tmp/.X11-unix/X0");
	} else {
		if (err < 0)
			return err;
		addr.sun_path[err] = '\0';
	}

#ifdef CONFIG_ABI_TRACE
	if (ibcs_trace & TRACE_SOCKSYS)
		printk(KERN_DEBUG "%d iBCS: SPX: %u get a Unix domain socket\n",
			current->pid, fd);
#endif
	args[0] = AF_UNIX;
	args[1] = SOCK_STREAM;
	args[2] = 0;
	set_fs(get_ds());
	newfd = SYS(socketcall)(SYS_SOCKET, args);
	set_fs(old_fs);
	if (newfd < 0)
		return newfd;

#ifdef CONFIG_ABI_TRACE
	if (ibcs_trace & TRACE_SOCKSYS)
		printk(KERN_DEBUG "%d iBCS: SPX: %u connect to \"%s\"\n",
			current->pid, fd, addr.sun_path);
#endif
	args[0] = newfd;
	args[1] = (int)&addr;
	args[2] = sizeof(struct sockaddr_un);
	set_fs(get_ds());
	err = SYS(socketcall)(SYS_CONNECT, args);
	set_fs(old_fs);
	if (err) {
		SYS(close)(newfd);
		return err;
	}
	return newfd;
}


int
abi_socksys_fd_init(int fd, int rw, const char *buf, int *count)
{
	struct file *filep;
	struct inode *ino;
	int error, sockfd;

	/* FIXME: we are always holding the filep when we get here
	 * anyway so we could just use fcheck here...
	 */
	filep = fget(fd);
	if (!filep)
		return -EBADF;
	ino = filep->f_dentry->d_inode;

#ifdef CONFIG_ABI_TRACE
	if (ibcs_trace & TRACE_SOCKSYS)
		printk(KERN_DEBUG
			"%d iBCS: socksys: fd=%d initializing\n",
			current->pid, fd);
#endif
	/* Minor = 0 is the socksys device itself. No special handling
	 *           will be needed as it is controlled by the application
	 *           via ioctls.
	 */
	if (MINOR(ino->i_rdev) == 0) {
		fput(filep);
		return 0;
	}

	/* Minor = 1 is the spx device. This is the client side of a
	 *           streams pipe to the X server. Under SCO and friends
	 *           the library code messes around setting the connection
	 *           up itself. We do it ourselves - this means we don't
	 *           need to worry about the implementation of the server
	 *           side (/dev/X0R - which must exist but can be a link
	 *           to /dev/null) nor do we need to actually implement
	 *           getmsg/putmsg.
	 */
	if (MINOR(ino->i_rdev) == 1) {
		int unit = 1;

		fput(filep);

		/* It seems early spx implementations were just a
		 * quick hack to get X to work. They only supported
		 * one destination and connected automatically.
		 * Later versions take a single byte write, the
		 * value of the byte telling them which destination
		 * to connect to. Hence this quick hack to work
		 * with both. If the first write is a single byte
		 * it's a connect request otherwise we auto-connect
		 * to destination 1.
		 */
		if (rw == 1 && *count == 1) {
			error = get_user(unit, buf);
			if (error)
				return error;
			(*count)--;
		}

		sockfd = spx_connect(fd, unit);
		if (sockfd < 0)
			return sockfd;
	}

	/*
	 * Otherwise the high 4 bits specify the address/protocol
	 * family (AF_INET, AF_UNIX etc.) and the low 4 bits determine
	 * the protocol (IPPROTO_IP, IPPROTO_UDP, IPPROTO_TCP etc.)
	 * although not using a one-to-one mapping as the minor number
	 * is not big enough to hold everything directly. The socket
	 * type is inferrred from the protocol.
	 */
	else { /* XTI */
		int args[3];
		mm_segment_t old_fs = get_fs();

		/* Grab a socket. */
#ifdef CONFIG_ABI_TRACE
		if (ibcs_trace & TRACE_SOCKSYS)
			printk(KERN_DEBUG
				"%d iBCS: XTI: %d get socket for transport end"
				" point (dev = 0x%04x)\n",
				current->pid, fd, ino->i_rdev);
#endif
		switch ((args[0] = ((MINOR(ino->i_rdev) >> 4) & 0x0f))) {
			case AF_UNIX:
				args[1] = SOCK_STREAM;
				args[2] = 0;
				break;

			case AF_INET: {
				int prot[16] = {
					IPPROTO_ICMP,	IPPROTO_ICMP,
					IPPROTO_IGMP,	IPPROTO_IPIP,
					IPPROTO_TCP,	IPPROTO_EGP,
					IPPROTO_PUP,	IPPROTO_UDP,
					IPPROTO_IDP,	IPPROTO_RAW,
				};
				int type[16] = {
					SOCK_RAW,	SOCK_RAW,
					SOCK_RAW,	SOCK_RAW,
					SOCK_STREAM,	SOCK_RAW,
					SOCK_RAW,	SOCK_DGRAM,
					SOCK_RAW,	SOCK_RAW,
				};
				int i = MINOR(ino->i_rdev) & 0x0f;
				args[2] = prot[i];
				args[1] = type[i];
				break;
			}

			default:
				args[1] = SOCK_RAW;
				args[2] = 0;
				break;
		}
		fput(filep);
#ifdef CONFIG_ABI_TRACE
		if (ibcs_trace & TRACE_SOCKSYS)
			printk(KERN_DEBUG
				"%d iBCS: XTI: %d socket %d %d %d\n",
				current->pid, fd,
				args[0], args[1], args[2]);
#endif
		set_fs(get_ds());
		sockfd = SYS(socketcall)(SYS_SOCKET, args);
		set_fs(old_fs);
		if (sockfd < 0)
			return sockfd;
	}

	/* Redirect operations on the socket fd via our emulation
	 * handlers then swap the socket fd and the original fd,
	 * discarding the original fd.
	 */
	inherit_socksys_funcs(sockfd, TS_UNBND);
#ifdef CONFIG_ABI_TRACE
	if (ibcs_trace & TRACE_SOCKSYS)
		printk(KERN_DEBUG "%d iBCS: XTI: %d -> %d\n",
			current->pid, fd, sockfd);
#endif
	SYS(dup2)(sockfd, fd);
	SYS(close)(sockfd);
	return 1;
}
EXPORT_SYMBOL(abi_socksys_fd_init);


static int
socksys_read(struct file *filep, char *buf, size_t count, loff_t *offset)
{
	struct inode *ino = filep->f_dentry->d_inode;

	/* FIXME: this condition *must* be true - if we have already
	 * replaced this with a socket we have changed the file ops
	 * too surely?
	 */
	CHECK_LOCK();
	if (ino && !ino->i_sock) {
		int fd;
		for (fd=0; fd<FDS_TABLE_LEN; fd++) {
			if (fcheck(fd) == filep) {
				int error;
				error = abi_socksys_fd_init(fd, 0, NULL, NULL);
				if (error > 0) {
					fput(filep);
					filep = fget(fd);
					return filep->f_op->read(filep, buf,
								count, offset);
				} else
					return error ? error : -EINVAL;
			}
		}
	}
	return -EINVAL;
}


static int
socksys_write(struct file *filep, const char *buf, size_t count, loff_t *offset)
{
	struct inode *ino = filep->f_dentry->d_inode;

	/* FIXME: this condition *must* be true - if we have already
	 * replaced this with a socket we have changed the file ops
	 * too surely?
	 */
	CHECK_LOCK();
	if (ino && !ino->i_sock) {
		int fd;
		for (fd=0; fd<FDS_TABLE_LEN; fd++) {
			if (fcheck(fd) == filep) {
				int error;
				error = abi_socksys_fd_init(fd, 1, buf, &count);
				if (error > 0) {
					fput(filep);
					filep = fget(fd);
					return count
						? filep->f_op->write(filep, buf,
								count, offset)
						: 0;
				} else
					return error ? error : -EINVAL;
			}
		}
	}
	return -EINVAL;
}


int
socksys_syscall(int *sp)
{
	int error, cmd;

	error = get_user(cmd, sp);
	if (error)
		return error;
	sp++;

#ifdef CONFIG_ABI_TRACE
	if (ibcs_trace & TRACE_SOCKSYS) {
		unsigned long a0, a1, a2, a3, a4, a5;
		static char *cmd_map[] = {
			"", "accept", "bind", "connect", "getpeername",
			"getsockname", "getsockopt", "listen", "recv",
			"recvfrom", "send", "sendto", "setsockopt", "shutdown",
			"socket", "select", "getipdomain", "setipdomain",
			"adjtime", "setreuid", "setregid", "gettimeofday",
			"settimeofday", "getitimer", "setitimer",
			"recvmsg", "sendmsg", "sockpair"
		};

		get_user(a0, sp+0);
		get_user(a1, sp+1);
		get_user(a2, sp+2);
		get_user(a3, sp+3);
		get_user(a4, sp+4);
		get_user(a5, sp+5);
		printk(KERN_DEBUG "%d iBCS: socksys:"
			" %s (%d) <0x%lx,0x%lx,0x%lx,0x%lx,0x%lx,0x%lx>\n",
			current->pid,
			(cmd >= 0 && cmd < sizeof(cmd_map)/sizeof(cmd_map[0]))
				? cmd_map[cmd] : "???",
			cmd, a0, a1, a2, a3, a4, a5);
	}
#endif

	switch (cmd) {
		case SSYS_SO_SOCKET: {
			/* Get a socket but replace the socket file
			 * operations with our own so we can do the
			 * right thing for ioctls.
			 */
			int fd;
			unsigned long x;

			get_user(x, ((unsigned long *)sp)+0);
			put_user(map_value(current->exec_domain->af_map, x, 0),
 				((unsigned long *)sp)+0);
			get_user(x, ((unsigned long *)sp)+1);
			put_user(map_value(current->exec_domain->socktype_map, x, 0),
 				((unsigned long *)sp)+1);

			if ((fd = SYS(socketcall)(SYS_SOCKET, sp)) < 0)
				return fd;

			inherit_socksys_funcs(fd, TS_UNBND);
			return fd;
		}

		case SSYS_SO_ACCEPT: {
			int fd;

			if ((fd = SYS(socketcall)(SYS_ACCEPT, sp)) < 0)
				return fd;

			inherit_socksys_funcs(fd, TS_DATA_XFER);
			return fd;
		}
		case SSYS_SO_BIND:
			return SYS(socketcall)(SYS_BIND, sp);
		case SSYS_SO_CONNECT:
			return SYS(socketcall)(SYS_CONNECT, sp);
		case SSYS_SO_GETPEERNAME:
			return SYS(socketcall)(SYS_GETPEERNAME, sp);
		case SSYS_SO_GETSOCKNAME:
			return SYS(socketcall)(SYS_GETSOCKNAME, sp);
		case SSYS_SO_GETSOCKOPT:
			return abi_do_getsockopt((unsigned long *)sp);
		case SSYS_SO_LISTEN:
			return SYS(socketcall)(SYS_LISTEN, sp);
		case SSYS_SO_RECV: {
			int err = SYS(socketcall)(SYS_RECV, sp);
			if (err == -EAGAIN) err = -EWOULDBLOCK;
			return err;
		}
		case SSYS_SO_RECVFROM: {
			int err = SYS(socketcall)(SYS_RECVFROM, sp);
			if (err == -EAGAIN) err = -EWOULDBLOCK;
			return err;
		}
		case SSYS_SO_SEND: {
			int err = SYS(socketcall)(SYS_SEND, sp);
			if (err == -EAGAIN) err = -EWOULDBLOCK;
			return err;
		}
		case SSYS_SO_SENDTO: {
			int err = SYS(socketcall)(SYS_SENDTO, sp);
			if (err == -EAGAIN) err = -EWOULDBLOCK;
			return err;
		}
		case SSYS_SO_SETSOCKOPT:
			return abi_do_setsockopt((unsigned long *)sp);
		case SSYS_SO_SHUTDOWN:
			return SYS(socketcall)(SYS_SHUTDOWN, sp);

		case SSYS_SO_GETIPDOMAIN: {
			int error, len;
			char *name, *p;

			error = get_user(name, (char *)(sp+0));
			if (!error)
				get_user(len, sp+1);
			if (!error) {
				down_read(&uts_sem);
				error = verify_area(VERIFY_WRITE, name, len);
				if (!error) {
					--len;
					for (p=system_utsname.nodename; *p && *p != '.'; p++);
					if (*p == '.')
						p++;
					else
						p = system_utsname.domainname;
					if (strcmp(p, "(none)"))
						for (;*p && len > 0; p++,len--) {
							__put_user(*p, name);
							name++;
						}
					__put_user('\0', name);
				}
				up_read(&uts_sem);
			}
			return error;
		}
		case SSYS_SO_SETIPDOMAIN: {
			int error, len, togo;
			char *name, *p;

			if (!capable(CAP_SYS_ADMIN))
				return -EPERM;

			error = get_user(name, (char *)(sp+0));
			if (!error)
				error = get_user(len, sp+1);
			if (error)
				return error;

			down_write(&uts_sem);
			togo = __NEW_UTS_LEN;
			for (p=system_utsname.nodename; *p && *p != '.'; p++,togo--);
			if (*p == '.')
				p++,togo--;

			error = -EINVAL;
			if (len <= togo) {
				while (len-- > 0) {
					get_user(*p, name);
					p++;
					name++;
				}
				*p = '\0';
				error = 0;
			}
			up_write(&uts_sem);
			return error;
		}

		case SSYS_SO_SETREUID:
		case SSYS_SO_SETREGID: {
			int error;
			uid_t ruid, euid;

			error = get_user(ruid, sp+0);
			if (!error)
				error = get_user(euid, sp+1);
			if (error)
				return error;
			return (cmd == SSYS_SO_SETREUID)
				? SYS(setreuid)(ruid, euid)
				: SYS(setregid)(ruid, euid);
		}

		case SSYS_SO_GETTIME:
		case SSYS_SO_SETTIME: {
			int error;
			struct timeval *tv;
			struct timezone *tz;

			error = get_user(tv, sp+0);
			if (!error)
				error = get_user(tz, sp+1);
			if (error)
				return error;
			return (cmd == SSYS_SO_GETTIME)
				? SYS(gettimeofday)(tv, tz)
				: SYS(settimeofday)(tv, tz);
		}

		case SSYS_SO_GETITIMER: {
			int error, which;
			struct itimerval *value;

			error = get_user(which, sp+0);
			if (!error)
				error = get_user(value, sp+1);
			if (error)
				return error;
			return SYS(getitimer)(which, value);
		}
		case SSYS_SO_SETITIMER: {
			int error, which;
			struct itimerval *value, *ovalue;

			error = get_user(which, sp+0);
			if (!error)
				error = get_user(value, sp+1);
			if (!error)
				error = get_user(ovalue, sp+2);
			if (error)
				return error;
			return SYS(setitimer)(which, value, ovalue);
		}

		case SSYS_SO_SELECT:
			/* This may be wrong? I don't know how to trigger
			 * this case. Select seems to go via the Xenix
			 * select entry point.
			 */
			return SYS(select)(sp);

		case SSYS_SO_ADJTIME:
			return -EINVAL;

		/* These appear in SCO 3.2v5. I assume that the format of
		 * a msghdr is identical with Linux. I have not checked.
		 */
		case SSYS_SO_RECVMSG: {
			int err = SYS(socketcall)(SYS_RECVMSG, sp);
			if (err == -EAGAIN) err = -EWOULDBLOCK;
			return err;
		}
		case SSYS_SO_SENDMSG: {
			int err = SYS(socketcall)(SYS_SENDMSG, sp);
			if (err == -EAGAIN) err = -EWOULDBLOCK;
			return err;
		}

		case SSYS_SO_SOCKPAIR: {
			/* Get a socketpair but replace the socket file
			 * operations with our own so we can do the
			 * right thing for ioctls.
			 */
			struct file *filep;
			struct inode *ino;
			int error, args[4], pairin[2], pairout[2];
			mm_segment_t old_fs = get_fs();

			/* The first two arguments are file descriptors
			 * of sockets which have already been opened
			 * and should now be connected back to back.
			 */
			error = get_user(pairin[0], sp+0);
			if (!error)
				error = get_user(pairin[1], sp+1);
			if (error)
				return error;

			filep = fget(pairin[0]);
			if (!filep)
				return -EBADF;
			ino = filep->f_dentry->d_inode;
			if (!ino || !ino->i_sock) {
				fput(filep);
				return -EBADF;
			}

			args[0] = AF_UNIX;
			args[1] = ino->u.socket_i.type;
			args[2] = 0;
			args[3] = (int)pairout;

			fput(filep);

			/* FIXME: Do we need to close these here? If we
			 * fail to connect them should they be open?
			 */
			SYS(close)(pairin[0]);
			SYS(close)(pairin[1]);

			set_fs(get_ds());
			error = SYS(socketcall)(SYS_SOCKETPAIR, args);
			set_fs(old_fs);
			if (error < 0)
				return error;

			if (pairout[0] != pairin[0]) {
				SYS(dup2)(pairout[0], pairin[0]);
				SYS(close)(pairout[0]);
			}
			if (pairout[1] != pairin[1]) {
				SYS(dup2)(pairout[1], pairin[1]);
				SYS(close)(pairout[1]);
			}

			inherit_socksys_funcs(pairin[0], TS_DATA_XFER);
			inherit_socksys_funcs(pairin[1], TS_DATA_XFER);
			return 0;
		}
	}

	return -EINVAL;
}


EXPORT_SYMBOL(socksys_syscall);

int
abi_ioctl_socksys(int fd, unsigned int cmd, void *arg)
{
	int error;

	switch (cmd) {
		/* Strictly the ip domain and nis domain are separate and
		 * distinct under SCO but Linux only has the one domain.
		 */
		case NIOCGETDOMNAM: {
			struct domnam_args dn;
			char *p;

			error = copy_from_user(&dn, (char *)arg,
					sizeof(struct domnam_args));
			if (error)
				return -EFAULT;

			down_read(&uts_sem);
			error = verify_area(VERIFY_WRITE, dn.name, dn.namelen);
			if (!error) {
				--dn.namelen;
				for (p=system_utsname.domainname; *p && dn.namelen > 0; p++,dn.namelen--) {
					__put_user(*p, dn.name);
					dn.name++;
				}
				__put_user('\0', dn.name);
			}
			up_read(&uts_sem);
			return error;
		}
		case NIOCSETDOMNAM: {
			struct domnam_args dn;

			error = copy_from_user(&dn, (char *)arg,
					sizeof(struct domnam_args));
			if (error)
				return -EFAULT;

			return SYS(setdomainname)(dn.name, dn.namelen);
		}

		case NIOCLSTAT: {
			/* I think this was used before symlinks were added
			 * to the base SCO OS?
			 */
			struct lstat_args st;

			error = copy_from_user(&st, (char *)arg,
					sizeof(struct lstat_args));
			if (error)
				return -EFAULT;

			return abi_lstat(st.fname, st.statb);
		}

		case NIOCOLDGETFH:
		case NIOCGETFH: {
			struct getfh_args gf;
			struct dentry *dentry;

			if (!capable(CAP_SYS_ADMIN))
				return -EPERM;

			error = copy_from_user(&gf, (char *)arg,
						sizeof(struct getfh_args));
			if (error)
				return -EFAULT;

			lock_kernel();
			error = verify_area(VERIFY_WRITE, (char *)gf.fhp, sizeof(fhandle_t));
			if (!error) {
				dentry = namei(gf.fname);
				error = PTR_ERR(dentry);
				if (!IS_ERR(dentry)) {
					struct inode *ino = dentry->d_inode;
					__put_user(ino->i_dev, &gf.fhp->fh.fsid);
					__put_user(ino->i_ino, &gf.fhp->fh.fno);
					__put_user(0L, &gf.fhp->fh.fgen);
					__put_user(ino->i_dev, &gf.fhp->fh.ex_fsid);
					__put_user(ino->i_ino, &gf.fhp->fh.ex_fno);
					__put_user(0L, &gf.fhp->fh.ex_fgen);
					dput(dentry);
					error = 0;
				}
			}
			unlock_kernel();
			return error;
		}

		case NIOCNFSD:
		case NIOCASYNCD:
		case NIOCCLNTHAND:
		case NIOCEXPORTFS:
			return -EINVAL;

		case SSYS_SIOCSOCKSYS:		/* Pseudo socket syscall */
		case SVR4_SIOCSOCKSYS:
			return socksys_syscall((int *)arg);

		case SSYS_SIOCSHIWAT:		/* set high watermark */
		case SVR4_SIOCSHIWAT:
		case SSYS_SIOCSLOWAT:		/* set low watermark */
		case SVR4_SIOCSLOWAT:
			/* Linux doesn't support them but lie anyway
			 * or some things take it as fatal (why?)
			 * FIXME: actually we can do this now...
			 */
			return 0;

		case SSYS_SIOCGHIWAT:		/* get high watermark */
		case SVR4_SIOCGHIWAT:
		case SSYS_SIOCGLOWAT:		/* get low watermark */
		case SVR4_SIOCGLOWAT:
			/* Linux doesn't support them but lie anyway
			 * or some things take it as fatal (why?)
			 * FIXME: actually we can do this now...
			 */
			if ((error = verify_area(VERIFY_WRITE, (char *)arg,
						sizeof(unsigned long))))
				return error;
			put_user(0, (unsigned long *)arg);
			return 0;

		case SSYS_SIOCATMARK:		/* at oob mark? */
		case SVR4_SIOCATMARK:
			return SYS(ioctl)(fd, SIOCATMARK, arg);

		case SSYS_SIOCSPGRP:		/* set process group */
		case SVR4_SIOCSPGRP:
			return SYS(ioctl)(fd, SIOCSPGRP, arg);
		case SSYS_SIOCGPGRP:		/* get process group */
		case SVR4_SIOCGPGRP:
			return SYS(ioctl)(fd, SIOCGPGRP, arg);

		case FIONREAD:
		case SSYS_FIONREAD:		/* BSD compatibilty */
			error = SYS(ioctl)(fd, TIOCINQ, arg);
#ifdef CONFIG_ABI_TRACE
			if (!error && (ibcs_trace & TRACE_SOCKSYS)) {
				unsigned long n;
				get_user(n, (unsigned long *)arg);
				printk(KERN_DEBUG
					"%d iBCS: socksys: %d FIONREAD found %lu bytes ready\n",
					current->pid, fd, n);
			}
#endif
			return error;

		case SSYS_FIONBIO: 		/* BSD compatibilty */
			return SYS(ioctl)(fd, FIONBIO, arg);

		case SSYS_FIOASYNC: 		/* BSD compatibilty */
			return SYS(ioctl)(fd, FIOASYNC, arg);

		case SSYS_SIOCADDRT:		/* add route */
		case SVR4_SIOCADDRT:
			return SYS(ioctl)(fd, SIOCADDRT, arg);
		case SSYS_SIOCDELRT:		/* delete route */
		case SVR4_SIOCDELRT:
			return SYS(ioctl)(fd, SIOCDELRT, arg);

		case SSYS_SIOCSIFADDR:		/* set ifnet address */
		case SVR4_SIOCSIFADDR:
			return SYS(ioctl)(fd, SIOCSIFADDR, arg);
		case SSYS_SIOCGIFADDR:		/* get ifnet address */
		case SVR4_SIOCGIFADDR:
			return SYS(ioctl)(fd, SIOCGIFADDR, arg);

		case SSYS_SIOCSIFDSTADDR:	/* set p-p address */
		case SVR4_SIOCSIFDSTADDR:
			return SYS(ioctl)(fd, SIOCSIFDSTADDR, arg);
		case SSYS_SIOCGIFDSTADDR:	/* get p-p address */
		case SVR4_SIOCGIFDSTADDR:
			return SYS(ioctl)(fd, SIOCGIFDSTADDR, arg);

		case SSYS_SIOCSIFFLAGS:		/* set ifnet flags */
		case SVR4_SIOCSIFFLAGS:
			return SYS(ioctl)(fd, SIOCSIFFLAGS, arg);
		case SSYS_SIOCGIFFLAGS:		/* get ifnet flags */
		case SVR4_SIOCGIFFLAGS:
#if 0
		case SVRX_SIOCGIFFLAGS:
#endif
			return SYS(ioctl)(fd, SIOCGIFFLAGS, arg);

		case SSYS_SIOCGIFCONF:		/* get ifnet list */
		case SVR4_SIOCGIFCONF:
#if 0
		case SVRX_SIOCGIFCONF:
#endif
			return SYS(ioctl)(fd, SIOCGIFCONF, arg);

		case SSYS_SIOCGIFBRDADDR:	/* get broadcast addr */
		case SVR4_SIOCGIFBRDADDR:
			return SYS(ioctl)(fd, SIOCGIFBRDADDR, arg);
		case SSYS_SIOCSIFBRDADDR:	/* set broadcast addr */
		case SVR4_SIOCSIFBRDADDR:
			return SYS(ioctl)(fd, SIOCSIFBRDADDR, arg);

		case SSYS_SIOCGIFNETMASK:	/* get net addr mask */
		case SVR4_SIOCGIFNETMASK:
			return SYS(ioctl)(fd, SIOCGIFNETMASK, arg);
		case SSYS_SIOCSIFNETMASK:	/* set net addr mask */
			return SYS(ioctl)(fd, SIOCSIFNETMASK, arg);

		case SSYS_SIOCGIFMETRIC:	/* get IF metric */
		case SVR4_SIOCGIFMETRIC:
			return SYS(ioctl)(fd, SIOCGIFMETRIC, arg);
		case SSYS_SIOCSIFMETRIC:	/* set IF metric */
		case SVR4_SIOCSIFMETRIC:
			return SYS(ioctl)(fd, SIOCSIFMETRIC, arg);

		case SSYS_SIOCSARP:		/* set arp entry */
		case SVR4_SIOCSARP:
			return SYS(ioctl)(fd, SIOCSARP, arg);
		case SSYS_SIOCGARP:		/* get arp entry */
		case SVR4_SIOCGARP:
			return SYS(ioctl)(fd, SIOCGARP, arg);
		case SSYS_SIOCDARP:		/* delete arp entry */
		case SVR4_SIOCDARP:
			return SYS(ioctl)(fd, SIOCDARP, arg);

		case SSYS_SIOCGENADDR:		/* Get ethernet addr */
		case SVR4_SIOCGENADDR:
			return SYS(ioctl)(fd, SIOCGIFHWADDR, arg);

		case SSYS_SIOCSIFMTU:		/* get if_mtu */
		case SVR4_SIOCSIFMTU:
			return SYS(ioctl)(fd, SIOCSIFMTU, arg);
		case SSYS_SIOCGIFMTU:		/* set if_mtu */
		case SVR4_SIOCGIFMTU:
			return SYS(ioctl)(fd, SIOCGIFMTU, arg);

		case SSYS_SIOCGETNAME:		/* getsockname */
		case SVR4_SIOCGETNAME:
		case SSYS_SIOCGETPEER: 		/* getpeername */
		case SVR4_SIOCGETPEER:
		{
			struct sockaddr uaddr;
			int uaddr_len = sizeof(struct sockaddr);
			int op, args[3];
			mm_segment_t old_fs;

			if ((error = verify_area(VERIFY_WRITE, (char *)arg, sizeof(struct sockaddr))))
				return error;
			if (cmd == SSYS_SIOCGETNAME || cmd == SVR4_SIOCGETNAME)
				op = SYS_GETSOCKNAME;
			else
				op = SYS_GETPEERNAME;
			args[0] = fd;
			args[1] = (int)&uaddr;
			args[2] = (int)&uaddr_len;
			old_fs = get_fs();
			set_fs (get_ds());
			error = SYS(socketcall)(op, args);
			set_fs(old_fs);
			if (error >= 0)
				copy_to_user((char *)arg, &uaddr, uaddr_len);
			return error;
		}

		case SSYS_IF_UNITSEL:		/* set unit number */
		case SVR4_IF_UNITSEL:
		case SSYS_SIOCXPROTO:		/* empty proto table */
		case SVR4_SIOCXPROTO:

		case SSYS_SIOCIFDETACH:		/* detach interface */
		case SVR4_SIOCIFDETACH:
		case SSYS_SIOCGENPSTATS:	/* get ENP stats */
		case SVR4_SIOCGENPSTATS:

		case SSYS_SIOCSIFNAME:		/* set interface name */
		case SVR4_SIOCSIFNAME:
		case SSYS_SIOCGIFONEP:		/* get one-packet params */
		case SSYS_SIOCSIFONEP:		/* set one-packet params */

		case SSYS_SIOCPROTO:		/* link proto */
		case SVR4_SIOCPROTO:
		case SSYS_SIOCX25XMT:
		case SVR4_SIOCX25XMT:
		case SSYS_SIOCX25RCV:
		case SVR4_SIOCX25RCV:
		case SSYS_SIOCX25TBL:
		case SVR4_SIOCX25TBL:

		default:
			printk(KERN_DEBUG "%d iBCS: socksys: %d: ioctl 0x%x with argument 0x%lx requested\n",
				current->pid, fd,
				cmd, (unsigned long)arg);
			break;
	}

	return -EINVAL;
}

EXPORT_SYMBOL(abi_ioctl_socksys);


static int
socksys_open(struct inode *ino, struct file *filep)
{
	//	MOD_INC_USE_COUNT;
#ifdef CONFIG_ABI_TRACE
	if (ibcs_trace & TRACE_SOCKSYS)
		printk(KERN_DEBUG
			"%d iBCS: socksys: filep=0x%08lx, inode=0x%08lx opening\n",
			current->pid, (unsigned long)filep, (unsigned long)ino);
#endif
	return 0;
}


static unsigned int
socksys_poll(struct file *filep, struct poll_table_struct *wait)
{
	unsigned int mask = 0;

#ifdef CONFIG_ABI_XTI
	struct inode *ino = filep->f_dentry->d_inode;

	/* If this is a timod transport end point and there
	 * is a control message queued we have readable data.
	 */
	if (ino && ino->i_sock && MINOR(ino->i_rdev) != 1
	&& Priv(filep) && Priv(filep)->pfirst)
		mask = Priv(filep)->pfirst->pri == MSG_HIPRI
			? POLLPRI
			: POLLIN;
#endif

	if (sock_poll)
		mask |= (*sock_poll)(filep, wait);
	return mask;
}


static int
socksys_close(struct inode *ino, struct file *filep)
{
	int error;

	/* Not being a socket is not an error - it is probably
	 * just the pseudo device transport provider.
	 */
	error = 0;
	if (ino && ino->i_sock) {
#ifdef CONFIG_ABI_XTI
		if (filep->private_data) {
			struct T_primsg *it;
			it = ((struct T_private *)filep->private_data)->pfirst;
			while (it) {
				struct T_primsg *tmp = it;
				it = it->next;
				kfree(tmp);
			}
			kfree(filep->private_data);
		}
#endif
		error = sock_close(ino, filep);
	}

#ifdef CONFIG_ABI_TRACE
	if (ibcs_trace & TRACE_SOCKSYS)
		printk(KERN_DEBUG "%d iBCS: socksys: %lx closed\n",
			current->pid, (unsigned long)filep);
#endif
	//	MOD_DEC_USE_COUNT;
	return error;
}

static int __init init_abi(void)
{
	/* N.B. this is coded to allow for the possibility of auto
	 * assignment of major number by passing 0 and seeing what we
	 * get back. This isn't possible since I haven't reworked the
	 * device subsystems yet :-).
	 */
	socksys_major = register_chrdev(SOCKSYS_MAJOR, "socksys", &socksys_fops);
	if (socksys_major < 0) {
		printk(KERN_ERR "iBCS: couldn't register socksys on character major %d\n",
			SOCKSYS_MAJOR);
		return socksys_major;
	} else {
		if (!socksys_major)
			socksys_major = SOCKSYS_MAJOR;
		printk(KERN_INFO "iBCS: socksys registered on character major %d\n", 
			socksys_major);
	}
	if (abi_proc_init()) {
		unregister_chrdev(socksys_major, "socksys");
		return 1;
	}
	return 0;
}

static void __exit cleanup_abi(void)
{
	/* Remove the socksys socket interface to streams based TCP/IP */
	if (socksys_major > 0 && unregister_chrdev(socksys_major, "socksys") != 0)
		printk(KERN_ERR "iBCS: couldn't unregister socksys device!\n");
	abi_proc_cleanup();
}

module_init(init_abi);
module_exit(cleanup_abi);
