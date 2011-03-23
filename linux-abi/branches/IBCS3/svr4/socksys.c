/*
 * socksys.c - SVR4 /dev/socksys emulation
 *
 * Copyright (c) 1994-1996 Mike Jagdis (jaggy@purplet.demon.co.uk)
 * Copyright (c) 2001 Caldera Deutschland GmbH
 * Copyright (c) 2001 Christoph Hellwig
 */

//#ident "%W% %G%"

#include "../include/util/i386_std.h"
//#include <linux/config.h>
#include <linux/module.h>

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
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/un.h>
#include <linux/utsname.h>
#include <linux/time.h>
#include <linux/termios.h>
#include <linux/sys.h>
#include <linux/file.h>
#include <linux/poll.h>
#include <linux/capability.h>
#include <linux/personality.h>
#include <linux/init.h>
#include <linux/syscalls.h>
#include <linux/namei.h>

#include <asm/uaccess.h>

#include "../include/stream.h"
#include "../include/socksys.h"
#include "../include/svr4/sockio.h"
#include "../include/svr4/sysent.h"
#include "../include/tli.h"

#include "../include/util/map.h"
#include "../include/util/trace.h"
#include "../include/util/revalidate.h"
#if _KSL > 18
#define system_utsname init_uts_ns.name
#endif
#if _KSL < 14
#define files_fdtable(x) x
#endif
#if _KSL > 25
#include <linux/fdtable.h>
#endif


/*
 * External declarations.
 */
struct svr4_stat;

/*
 * Forward declarations.
 */
static int	socksys_open(struct inode *ip, struct file *fp);
#ifdef CONFIG_65BIT
#define sockf_t static long
#else
#define sockf_t static int
#endif

#if defined(CONFIG_ABI_XTI)
static int	socksys_release(struct inode *ip, struct file *fp);
static u_int	socksys_poll(struct file *fp, struct poll_table_struct *wait);
#endif

ssize_t	socksys_read(struct file *fp, char __user *buf, size_t count, loff_t *ppos);
ssize_t	socksys_write(struct file *fp, const char __user *buf, size_t count, loff_t *ppos);

/*
 * The socksys socket file operations.
 * This gets filled in on module initialization.
 */
static struct file_operations socksys_socket_fops = {
	/* NOTHING */
};

/*
 * File operations for the user-visible device files.
 *
 * While open the files are handled as sockets.
 */
static struct file_operations socksys_fops = {
	owner:		THIS_MODULE,
	open:		socksys_open,
	read:		socksys_read,
	write:		socksys_write,
#ifdef CONFIG_ABI_XTI
	poll:		socksys_poll,
	release:	socksys_release,
#endif
};
static int (*abi_sock_close)(struct inode *,struct file *);
static unsigned int (*abi_sock_poll)(struct file *,struct poll_table_struct *);


void
inherit_socksys_funcs(u_int fd, int state)
{
	struct file		*fp;
	struct inode		*ip;
#ifdef CONFIG_ABI_XTI
	struct T_private	*tp;
#endif
        struct socket           *sp;
return;
	fp = fget(fd);
	if (fp == NULL)
		return;
	ip = fp->f_dentry->d_inode;

	/*
	 * SYSV sockets are BSD like with respect to ICMP errors
	 * with UDP rather than RFC conforming. I think.
	 */
        sp = SOCKET_I(ip); /* inode -> socket
	if (sp->sk) 
		sock_set_flag(sp->sk, SOCK_BSDISM);*/

	ip->i_mode = 0020000;
	ip->i_rdev = MKDEV(SOCKSYS_MAJOR, 0);

#ifdef CONFIG_ABI_XTI
	tp = kmalloc(sizeof(struct T_private), GFP_KERNEL);
	if (tp) {
		tp->magic = XTI_MAGIC;
		tp->state = state;
		tp->offset = 0;
		tp->pfirst = NULL;
		tp->plast = NULL;
	}
	fp->private_data = tp;
#endif
	if (socksys_socket_fops.read != fp->f_op->read) {
		memcpy(&socksys_socket_fops,fp->f_op,sizeof(struct file_operations));
		abi_sock_close = socksys_socket_fops.release;
		abi_sock_poll  = socksys_socket_fops.poll;
#ifdef CONFIG_ABI_XTI
		socksys_socket_fops.release = socksys_release;
		socksys_socket_fops.poll = socksys_poll;
#endif
	}
	fp->f_op = &socksys_socket_fops;
	fput(fp);
}

static int
spx_connect(u_int fd, int spxnum)
{
	struct sockaddr_un	sun;
	int			newfd, err;
	mm_segment_t		fs;
#ifndef CONFIG_65BIT
	int			a[3];
#endif

#if defined(CONFIG_ABI_TRACE)
	abi_trace(ABI_TRACE_SOCKSYS,
			"SPX: %u choose service %d\n", fd, spxnum);
#endif

	/*
	 * Rather than use an explicit path to the X :0 server
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
	sun.sun_family = AF_UNIX;
	sprintf(sun.sun_path, "/dev/spx.map/%u", spxnum);

	fs = get_fs();
	set_fs(get_ds());
	err = SYS(readlink,sun.sun_path, sun.sun_path, strlen(sun.sun_path));
	set_fs(fs);

	if (err == -ENOENT) {
#if defined(CONFIG_ABI_TRACE)
		abi_trace(ABI_TRACE_SOCKSYS,
			"SPX: %u no symlink \"%s\", try X :0\n",
			fd, sun.sun_path);
#endif
		strcpy(sun.sun_path, "/tmp/.X11-unix/X0");
	} else if (err < 0) {
#if defined(CONFIG_ABI_TRACE)
		abi_trace(ABI_TRACE_SOCKSYS,
			"SPX: readlink failed with %d\n", err);
#endif
		return (err);
	} else
		sun.sun_path[err] = '\0';

	set_fs(get_ds());
#ifdef CONFIG_65BIT
	newfd = SYS(socket, AF_UNIX, SOCK_STREAM, 0);
#else
	a[0]=AF_UNIX; a[1]=SOCK_STREAM; a[2]=0;
	newfd = SYS(socketcall,SYS_SOCKET,a);
#endif
	set_fs(fs);

	if (newfd < 0) {
#if defined(CONFIG_ABI_TRACE)
		abi_trace(ABI_TRACE_SOCKSYS,
			"SPX: %u got no UNIX domain socket (err=%d)\n",
			fd, err);
#endif
		return (newfd);
	}

#if defined(CONFIG_ABI_TRACE)
	abi_trace(ABI_TRACE_SOCKSYS,
		"SPX: %u got a UNIX domain socket\n", fd);
#endif

	set_fs(get_ds());
#ifdef CONFIG_65BIT
	err = SYS(connect, newfd, &sun, sizeof(struct sockaddr_un));
#else
	a[0]=newfd; a[1]= (int)((long)(&sun)); a[2]= sizeof(struct sockaddr_un);
	err = SYS(socketcall,SYS_CONNECT,a);
#endif
	set_fs(fs);

	if (err) {
#if defined(CONFIG_ABI_TRACE)
		abi_trace(ABI_TRACE_SOCKSYS,
			"SPX: %u connect to \"%s\" failed (err = %d)\n",
			fd, sun.sun_path, err);
#endif
		SYS(close,newfd);
		return (err);
	}

#if defined(CONFIG_ABI_TRACE)
	abi_trace(ABI_TRACE_SOCKSYS,
		"SPX: %u connect to \"%s\"\n",
		fd, sun.sun_path);
#endif
	return (newfd);
}

/*
 * XTI to Linux protocol table.
 */
static int inet_prot[16] = {
	IPPROTO_ICMP,	IPPROTO_ICMP,
	IPPROTO_IGMP,	IPPROTO_IPIP,
	IPPROTO_TCP,	IPPROTO_EGP,
	IPPROTO_PUP,	IPPROTO_UDP,
	IPPROTO_IDP,	IPPROTO_RAW,
};

static int inet_type[16] = {
	SOCK_RAW,	SOCK_RAW,
	SOCK_RAW,	SOCK_RAW,
	SOCK_STREAM,	SOCK_RAW,
	SOCK_RAW,	SOCK_DGRAM,
	SOCK_RAW,	SOCK_RAW,
};


static int
xti_connect(struct file *fp, u_int fd, dev_t dev)
{
	int			family, type, prot = 0, i, s;
	mm_segment_t		fs;
#ifndef CONFIG_65BIT
	int			a[3];
#endif

#if defined(CONFIG_ABI_TRACE)
	abi_trace(ABI_TRACE_SOCKSYS,
		"XTI: %d get socket for transport end point "
		"(dev = 0x%04x)\n", fd, dev);
#endif

	switch ((family = ((MINOR(dev) >> 4) & 0x0f))) {
	case AF_UNIX:
		type = SOCK_STREAM;
		break;
	case AF_INET:
		i = MINOR(dev) & 0x0f;
		type = inet_type[i];
		prot = inet_prot[i];
		break;
	default:
		type = SOCK_RAW;
		break;
	}

	fput(fp);

#if defined(CONFIG_ABI_TRACE)
	abi_trace(ABI_TRACE_SOCKSYS,
		"XTI: %d socket %d %d %d\n",
		fd, family, type, prot);
#endif

	fs = get_fs();
	set_fs(get_ds());
#ifdef CONFIG_65BIT
	s = SYS(socket, family, type, prot);
#else
	a[0]=family; a[1]=type; a[2]=prot;
	s = SYS(socketcall,SYS_SOCKET,a);
#endif
	set_fs(fs);

	return (s);
}

int
socksys_fdinit(int fd, int rw, const char *buf, int *count)
{
	struct file		*fp;
	struct inode		*ip;
	int			sockfd, error = -EINVAL;

#if defined(CONFIG_ABI_TRACE)
	abi_trace(ABI_TRACE_SOCKSYS, "socksys: fd=%d initializing\n", fd);
#endif

	fp = fget(fd);
	if (!fp)
		return -EBADF;
	ip = fp->f_dentry->d_inode;

	/*
	 * Minor = 0 is the socksys device itself. No special handling
	 *           will be needed as it is controlled by the application
	 *           via ioctls.
	 */
	if (MINOR(ip->i_rdev) == 0)
		goto fput;

	/*
	 * Minor = 1 is the spx device. This is the client side of a
	 *           streams pipe to the X server. Under SCO and friends
	 *           the library code messes around setting the connection
	 *           up itself. We do it ourselves - this means we don't
	 *           need to worry about the implementation of the server
	 *           side (/dev/X0R - which must exist but can be a link
	 *           to /dev/null) nor do we need to actually implement
	 *           getmsg/putmsg.
	 */
	if (MINOR(ip->i_rdev) == 1) {
		int unit = 1;

		/*
		 * It seems early spx implementations were just a
		 * quick hack to get X to work. They only supported
		 * one destination and connected automatically.
		 * Later versions take a single byte write, the
		 * value of the byte telling them which destination
		 * to connect to. Hence this quick hack to work
		 * with both. If the first write is a single byte
		 * it's a connect request otherwise we auto-connect
		 * to destination 1.
		 */
#if 0
		if (rw == 1 && *count == 1) {
			error = get_user(unit, buf);
			if (error)
				goto fput;
			(*count)--;
		}
#endif

		fput(fp);

		sockfd = spx_connect(fd, unit);
	} else {
		/*
		 * Otherwise the high 4 bits specify the address/protocol
		 * family (AF_INET, AF_UNIX etc.) and the low 4 bits determine
		 * the protocol (IPPROTO_IP, IPPROTO_UDP, IPPROTO_TCP etc.)
		 * although not using a one-to-one mapping as the minor number
		 * is not big enough to hold everything directly. The socket
		 * type is inferrred from the protocol.
		 */
		sockfd = xti_connect(fp, fd, ip->i_rdev);
	}

	/*
	 * Give up if we weren't able to allocate a socket.
	 * There is no sense in plying our funny game without a new fd.
	 */
	if (sockfd < 0)
		return sockfd;

	/*
	 * Redirect operations on the socket fd via our emulation
	 * handlers then swap the socket fd and the original fd,
	 * discarding the original fd.
	 */
	inherit_socksys_funcs(sockfd, TS_UNBND);

#if defined(CONFIG_ABI_TRACE)
	abi_trace(ABI_TRACE_SOCKSYS, "XTI: %d -> %d\n", fd, sockfd);
#endif

	SYS(dup2,sockfd, fd);
	SYS(close,sockfd);
	return 1;

fput:
	fput(fp);
	return error;
}

static int
socksys_open(struct inode *ip, struct file *fp)
{
#if defined(CONFIG_ABI_TRACE)
	abi_trace(ABI_TRACE_SOCKSYS,
		"socksys: fp=0x%p, ip=0x%p opening\n", fp, ip);
#endif
	return 0;
}

#ifdef CONFIG_ABI_XTI
static int
socksys_release(struct inode *ip, struct file *fp)
{
	int			error = 0;

	/*
	 * Not being a socket is not an error - it is probably
	 * just the pseudo device transport provider.
	 */
	if (!ip || !S_ISSOCK(ip->i_mode))
		goto out;

	if (fp->private_data) {
		struct T_primsg	*it;

		it = ((struct T_private *)fp->private_data)->pfirst;
		while (it) {
			struct T_primsg *tmp = it;
			it = it->next;
			kfree(tmp);
		}
		kfree(fp->private_data);
	}
//	error = (*abi_sock_close)(ip, fp);
out:
#if defined(CONFIG_ABI_TRACE)
	abi_trace(ABI_TRACE_SOCKSYS, "socksys: %p closed\n", fp);
#endif
	return error;
}

static u_int
socksys_poll(struct file *fp, struct poll_table_struct *wait)
{
	struct inode		*ip = fp->f_dentry->d_inode;
	u_int			mask = 0;

	/*
	 * If this is a timod transport end point and there
	 * is a control message queued we have readable data.
	 */
	if (ip && S_ISSOCK(ip->i_mode) && MINOR(ip->i_rdev) != 1) {
		if (Priv(fp) && Priv(fp)->pfirst) {
			if (Priv(fp)->pfirst->pri == MSG_HIPRI)
				mask |= POLLPRI;
			else
				mask |= POLLIN;
		}
	}
//	mask |= (*abi_sock_poll)(fp, wait);
	return (mask);
}
#endif

ssize_t
socksys_read(struct file *fp, char __user *buf, size_t count, loff_t *ppos)
{
	int			fd, error;

	/*if (S_ISSOCK(fp->f_dentry->d_inode->i_mode))
		BUG();*/

	for (fd = 0; fd < files_fdtable(current->files)->max_fds; fd++) {
		if (fcheck(fd) == fp) {
			error = socksys_fdinit(fd, 0, NULL, NULL);
			if (error < 0)
				return error;
			fput(fp);
			fp = fget(fd);
			return fp->f_op->read(fp, buf, count, ppos);
		}
	}

	return -EINVAL;
}

ssize_t
socksys_write(struct file *fp, const char __user *buf, size_t count, loff_t *ppos)
{
	int			fd, error;

	/*if (S_ISSOCK(fp->f_dentry->d_inode->i_mode))
		BUG();*/

	for (fd = 0; fd < files_fdtable(current->files)->max_fds; fd++) {
		if (fcheck(fd) == fp) {
			error = socksys_fdinit(fd, 1, buf, (int *)(&count));
			if (error < 0)
				return error;
			fput(fp);
			fp = fget(fd);
			if (count == 1) return 1;
			return fp->f_op->write(fp, buf, count, ppos);
		}
	}

	return -EINVAL;
}


/*
 * Get a socket but replace the socket file
 * operations with our own so we can do the
 * right thing for ioctls.
 */
static int
socksys_socket(u_int *sp)
{
	u_int			x;
	int			fd;

	get_user(x, ((u_int *)sp)+0);
	put_user(map_value(current_thread_info()->exec_domain->af_map, x, 0), ((u_int *)sp)+0);
	get_user(x, ((u_int *)sp)+1);
	put_user(map_value(current_thread_info()->exec_domain->socktype_map, x, 0),  ((u_int *)sp)+1);

#ifdef CONFIG_65BIT
	fd = SYS(socket, sp[0], sp[1], sp[2]);
#else
	fd = SYS(socketcall,SYS_SOCKET, sp);
#endif
	if (fd >= 0)
		inherit_socksys_funcs(fd, TS_UNBND);
	return fd;
}

static int
socksys_accept(u_int *sp)
{
	int			fd;

#ifdef CONFIG_65BIT
	fd = SYS(accept, sp[0], sp[1], sp[2]);
#else
	fd = SYS(socketcall,SYS_ACCEPT, sp);
#endif
	if (fd >= 0)
		inherit_socksys_funcs(fd, TS_DATA_XFER);
	return fd;
}

static int
socksys_getipdomain(u_int *sp)
{
	char			*name, *p;
	int			error, len;
	u_long                  *ulptr;

	ulptr = (u_long *)&name;
	error = get_user(*ulptr, (char *)(sp+0));
	if (error)
		return error;

	get_user(len, sp+1);
	if (error)
		return error;

	down_read(&uts_sem);
	if (access_ok(VERIFY_WRITE, name, len)) {
		--len;
		for (p = system_utsname.nodename; *p && *p != '.'; p++)
			;
		if (*p == '.')
			p++;
		else
			p = system_utsname.domainname;

		if (strcmp(p, "(none)")) {
			for (; *p && len > 0; p++,len--) {
				__put_user(*p, name);
				name++;
			}
		}
		__put_user('\0', name);
		error = 0;
	}
	else
		error = -EFAULT;
	up_read(&uts_sem);
	return error;
}

static int
socksys_setipdomain(u_int *sp)
{
	char			*name, *p;
	int			error, len, togo;
	unsigned long		*ulptr;

	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;

	ulptr = (u_long *)&name;
	error = get_user(*ulptr, (char *)(sp+0));
	if (error)
		return error;

	error = get_user(len, sp+1);
	if (error)
		return error;

	down_write(&uts_sem);
	togo = __NEW_UTS_LEN;
	for (p = system_utsname.nodename; *p && *p != '.'; p++,togo--)
		;
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

static int
socksys_setreugid(int cmd, u_int *sp)
{
	uid_t			ruid, euid;
	int			error;

	error = get_user(ruid, sp+0);
	if (error)
		return error;

	error = get_user(euid, sp+1);
	if (error)
		return error;

	if (cmd == SSYS_SO_SETREUID) {error=SYS(setreuid,ruid, euid);}
	else {error=SYS(setregid,ruid, euid);}
	return error;
}

/*
 * Get a socketpair but replace the socket file
 * operations with our own so we can do the
 * right thing for ioctls.
 */
static int
socksys_socketpair(u_int *sp)
{
	struct file		*fp;
	struct inode		*ip;
	mm_segment_t		fs;
	int			pairin[2], pairout[2];
	int			error;
#ifndef CONFIG_65BIT
	int			a[4];
#endif

	/*
	 * The first two arguments are file descriptors
	 * of sockets which have already been opened
	 * and should now be connected back to back.
	 */
	error = get_user(pairin[0], sp+0);
	if (!error)
		error = get_user(pairin[1], sp+1);
	if (error)
		return error;

	fp = fget(pairin[0]);
	if (!fp)
		return -EBADF;
	ip = fp->f_dentry->d_inode;

	fput(fp); /* this looks boguos */
	if (!ip || !S_ISSOCK(ip->i_mode))
		return -EBADF;


	/*
	 * XXX Do we need to close these here?
	 * XXX If we fail to connect them should they be open?
	 */
	SYS(close,pairin[0]);
	SYS(close,pairin[1]);

	fs = get_fs();
	set_fs(get_ds());
#ifdef CONFIG_65BIT
	error = SYS(socketpair, AF_UNIX, SOCKET_I(ip)->type, 0, pairout);
#else
	a[0]=AF_UNIX; a[1]=SOCKET_I(ip)->type; a[2]= 0; a[3]= (int)((long)pairout);
	error = SYS(socketcall,SYS_SOCKETPAIR,a);
#endif
	set_fs(fs);

	if (error < 0)
		return error;

	if (pairout[0] != pairin[0]) {
		SYS(dup2,pairout[0], pairin[0]);
		SYS(close,pairout[0]);
	}

	if (pairout[1] != pairin[1]) {
		SYS(dup2,pairout[1], pairin[1]);
		SYS(close,pairout[1]);
	}

	inherit_socksys_funcs(pairin[0], TS_DATA_XFER);
	inherit_socksys_funcs(pairin[1], TS_DATA_XFER);
	return 0;
}

int
socksys_syscall(u_int *sp)
{
	int error, cmd;
	unsigned long *ulptr;

	error = get_user(cmd, sp);
	if (error)
		return error;
	sp++;

#if defined(CONFIG_ABI_TRACE)
	if (abi_traced(ABI_TRACE_SOCKSYS)) {
		u_long a0, a1, a2, a3, a4, a5;
		static const char * const cmd_map[] = {
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

		__abi_trace("socksys: %s (%d) "
				"<0x%lx,0x%lx,0x%lx,0x%lx,0x%lx,0x%lx>\n",
				(cmd >= 0 &&
				 cmd < sizeof(cmd_map) / sizeof(cmd_map[0]))
				? cmd_map[cmd] : "???", cmd,
				a0, a1, a2, a3, a4, a5);
	}
#endif

	switch (cmd) {
	case SSYS_SO_SOCKET:
		return socksys_socket(sp);
	case SSYS_SO_ACCEPT:
		return socksys_accept(sp);
	case SSYS_SO_BIND:
#ifdef CONFIG_65BIT
		error=SYS(bind, sp[0], sp[1], sp[2]);
#else
		error=SYS(socketcall,SYS_BIND, sp);
#endif
		return error;
	case SSYS_SO_CONNECT:
#ifdef CONFIG_65BIT
		error=SYS(connect, sp[0], sp[1], sp[2]);
#else
		error=SYS(socketcall,SYS_CONNECT, sp);
#endif
		return error;
	case SSYS_SO_GETPEERNAME:
#ifdef CONFIG_65BIT
		error=SYS(getpeername, sp[0], sp[1], sp[2]);
#else
		error=SYS(socketcall,SYS_GETPEERNAME, sp);
#endif
		return error;
	case SSYS_SO_GETSOCKNAME:
#ifdef CONFIG_65BIT
		error=SYS(getsockname, sp[0], sp[1], sp[2]);
#else
		error=SYS(socketcall,SYS_GETSOCKNAME, sp);
#endif
		return error;
	case SSYS_SO_GETSOCKOPT:
		return abi_do_getsockopt(sp);
	case SSYS_SO_LISTEN:
#ifdef CONFIG_65BIT
		error=SYS(listen, sp[0], sp[1]);
#else
		error=SYS(socketcall,SYS_LISTEN, sp);
#endif
		return error;
	case SSYS_SO_RECV:
#ifdef CONFIG_65BIT
		error=SYS(recvfrom, sp[0], sp[1], sp[2], sp[3], 0, 0);
#else
		error = SYS(socketcall,SYS_RECV, sp);
#endif
		if (error == -EAGAIN)
			return -EWOULDBLOCK;
		return error;
	case SSYS_SO_RECVFROM:
#ifdef CONFIG_65BIT
		error=SYS(recvfrom, sp[0], sp[1], sp[2], sp[3], sp[4], sp[5]);
#else
		error = SYS(socketcall,SYS_RECVFROM, sp);
#endif
		if (error == -EAGAIN)
			return -EWOULDBLOCK;
		return error;
	case SSYS_SO_SEND:
#ifdef CONFIG_65BIT
		error=SYS(sendto, sp[0], sp[1], sp[2], sp[3], 0, 0);
#else
		error = SYS(socketcall,SYS_SEND, sp);
#endif
		if (error == -EAGAIN)
			error = -EWOULDBLOCK;
		return error;
	case SSYS_SO_SENDTO:
#ifdef CONFIG_65BIT
		error=SYS(sendto, sp[0], sp[1], sp[2], sp[3], sp[4], sp[5]);
#else
		error = SYS(socketcall,SYS_SENDTO, sp);
#endif
		if (error == -EAGAIN)
			error = -EWOULDBLOCK;
		return error;
	case SSYS_SO_SETSOCKOPT:
		return abi_do_setsockopt(sp);
	case SSYS_SO_SHUTDOWN:
#ifdef CONFIG_65BIT
		error=SYS(shutdown, sp[0], sp[1]);
#else
		error=SYS(socketcall,SYS_SHUTDOWN, sp);
#endif
		return error;
	case SSYS_SO_GETIPDOMAIN:
		return socksys_getipdomain(sp);
	case SSYS_SO_SETIPDOMAIN:
		return socksys_setipdomain(sp);
	case SSYS_SO_SETREUID:
	case SSYS_SO_SETREGID:
		return socksys_setreugid(cmd, sp);
	case SSYS_SO_GETTIME:
	case SSYS_SO_SETTIME:
	{
		struct timeval *tv;
		struct timezone *tz;

		ulptr = (unsigned long *)&tv;
		if ((error = get_user(*ulptr, sp+0)))
			return error;
		ulptr = (unsigned long *)&tz;
		if ((error = get_user(*ulptr, sp+1)))
			return error;
		if (cmd == SSYS_SO_GETTIME) {error=SYS(gettimeofday,tv, tz);}
		else {error=SYS(settimeofday,tv, tz);}
			return error;
	}

	case SSYS_SO_GETITIMER:
	{
		int which;
		struct itimerval *value;

		ulptr = (unsigned long *)&which;
		if ((error = get_user(*ulptr, sp+0)))
			return error;
		ulptr = (unsigned long *)&value;
		if ((error = get_user(*ulptr, sp+1)))
			return error;
		error=SYS(getitimer,which, value); return error;
	}
	case SSYS_SO_SETITIMER:
	{
		int which;
		struct itimerval *value, *ovalue;

		ulptr = (unsigned long *)&which;
		if ((error = get_user(*ulptr, sp+0)))
			return error;
		ulptr = (unsigned long *)&value;
		if ((error = get_user(*ulptr, sp+1)))
			return error;
		ulptr = (unsigned long *)&ovalue;
		if ((error = get_user(*ulptr, sp+2)))
			return error;
		error=SYS(setitimer,which, value, ovalue); return error;
	}

#ifdef BUGGY
	case SSYS_SO_SELECT:
		/*
		 * This may be wrong? I don't know how to trigger
		 * this case. Select seems to go via the Xenix
		 * select entry point.
		 */
		error=SYS(select,sp); return error;
#endif

	case SSYS_SO_ADJTIME:
		return -EINVAL;

	/*
	 * These appear in SCO 3.2v5. I assume that the format of
	 * a msghdr is identical with Linux. I have not checked.
	 */
	case SSYS_SO_RECVMSG:
#ifdef CONFIG_65BIT
		error = SYS(recvmsg, sp[0], sp[1], sp[3]);
#else
		error = SYS(socketcall,SYS_RECVMSG, sp);
#endif
		if (error == -EAGAIN)
			error = -EWOULDBLOCK;
		return error;
	case SSYS_SO_SENDMSG:
#ifdef CONFIG_65BIT
		error = SYS(sendmsg, sp[0], sp[1], sp[3]);
#else
		error = SYS(socketcall,SYS_SENDMSG, sp);
#endif
		if (error == -EAGAIN)
			error = -EWOULDBLOCK;
		return error;
	case SSYS_SO_SOCKPAIR:
		return socksys_socketpair(sp);
	}

	return -EINVAL;
}

static int
socksys_getdomainname(caddr_t arg)
{
	struct domnam_args	dn;
	char			*p;
	int			error = 0;

	if (copy_from_user(&dn, arg, sizeof(struct domnam_args)))
		return -EFAULT;

	down_read(&uts_sem);
	if (access_ok(VERIFY_WRITE, dn.name, dn.namelen)) {
		--dn.namelen;
		for (p = system_utsname.domainname; *p && dn.namelen > 0; p++) {
			__put_user(*p, dn.name);
			dn.name++;
			dn.namelen--;
		}
		__put_user('\0', dn.name);
	}
	else
		error = -EFAULT;
	up_read(&uts_sem);
	return error;
}

static int
socksys_setdomainname(caddr_t arg)
{
	struct domnam_args	dn;
	int err;

	if (copy_from_user(&dn, arg, sizeof(struct domnam_args)))
		return -EFAULT;
	err = SYS(setdomainname,dn.name, dn.namelen); return err;
}

/*
 * I think this was used before symlinks were added
 * to the base SCO OS?
 */
static int
socksys_lstat(caddr_t arg)
{
	struct lstat_args	st;

	if (copy_from_user(&st, arg, sizeof(struct lstat_args)))
		return -EFAULT;
	return svr4_lstat(st.fname, st.statb);
}

static int
socksys_getfh(caddr_t arg)
{
	struct getfh_args	gf;
#if _KSL > 26
	struct path             path;
#else
	struct nameidata	nd;
#endif
	int			error;

	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;

	if (copy_from_user(&gf, arg, sizeof(struct getfh_args)))
		return -EFAULT;

	if (!access_ok(VERIFY_WRITE, gf.fhp, sizeof(fhandle_t)))
		return -EFAULT;

#if _KSL > 24
#if _KSL > 26
	error = user_path(gf.fname, &path);
	if (error)
		return error;
	error = do_revalidate(path.dentry);
	if (!error) {
		struct inode	*ip = path.dentry->d_inode;
#else
	error = user_path_walk(gf.fname, &nd);
	if (!error) {
		struct inode	*ip = nd.path.dentry->d_inode;
#endif
#else
	error = do_revalidate(nd.dentry);
	if (!error) {
		struct inode	*ip = nd.dentry->d_inode;
#endif
		__put_user(ip->i_rdev, &gf.fhp->fh.fsid);
		/* FIXME: do we need linux_to_svr4_ino_t(ip->i_ino)? */
		__put_user(ip->i_ino, &gf.fhp->fh.fno);
		__put_user(0L, &gf.fhp->fh.fgen);
		__put_user(ip->i_rdev, &gf.fhp->fh.ex_fsid);
		/* FIXME: do we need linux_to_svr4_ino_t(ip->i_ino)? */
		__put_user(ip->i_ino, &gf.fhp->fh.ex_fno);
		__put_user(0L, &gf.fhp->fh.ex_fgen);
		error = 0;
	}
#if _KSL < 25
	path_release(&nd);
#else
#if _KSL > 26
	path_put(&path);
#else
	path_put(&nd.path);
#endif
#endif
	return error;
}

static int
socksys_getpeername(int fd, caddr_t arg)
{
	struct sockaddr		uaddr;
	int			addrlen;
	mm_segment_t		fs;
	int			error;
#ifndef CONFIG_65BIT
	int			a[3];
#endif

	addrlen = sizeof(struct sockaddr);

	if (!access_ok(VERIFY_WRITE, arg, addrlen))
		return -EFAULT;

	fs = get_fs();
	set_fs(get_ds());
#ifdef CONFIG_65BIT
	error = SYS(getpeername, fd, &uaddr, &addrlen);
#else
	a[0]=fd; a[1]= (int)((long)(&uaddr)); a[2]= (int)((long)(&addrlen));
	error = SYS(socketcall,SYS_GETPEERNAME,a);
#endif
	set_fs(fs);

	if (error >= 0)
		if (copy_to_user(arg, &uaddr, addrlen))
			error = -EFAULT;
	return error;
}

static int
socksys_getsockname(int fd, caddr_t arg)
{
	struct sockaddr		uaddr;
	int			addrlen;
	mm_segment_t		fs;
	int			error;
#ifndef CONFIG_65BIT
	int			a[3];
#endif

	addrlen = sizeof(struct sockaddr);

	if (!access_ok(VERIFY_WRITE, arg, addrlen))
		return -EFAULT;

	fs = get_fs();
	set_fs(get_ds());
#ifdef CONFIG_65BIT
	error = SYS(getsockname, fd, &uaddr, &addrlen);
#else
	a[0]=fd; a[1]= (int)((long)(&uaddr)); a[2]= (int)((long)(&addrlen));
	error = SYS(socketcall,SYS_GETSOCKNAME,a);
#endif
	set_fs(fs);

	if (error >= 0)
		if (copy_to_user(arg, &uaddr, addrlen))
			error = -EFAULT;
	return error;
}

static int
socksys_gifonep(caddr_t data)
{
	return -EOPNOTSUPP;
}

static int
socksys_sifonep(caddr_t data)
{
#if 0
	struct svr4_ifreq	*ifr = (struct svr4_ifreq *)data;

	printk("SIOCSIFONEP (spsize = %x, spthresh = %x) not supported\n",
			ifr->svr4_ifr_onepacket.spsize,
			ifr->svr4_ifr_onepacket.spthresh);
#endif
	return -EOPNOTSUPP;
}

int
abi_ioctl_socksys(int fd, unsigned int cmd, caddr_t arg)
{
	int error;

	switch (cmd) {
	/*
	 * Strictly the ip domain and nis domain are separate and
	 * distinct under SCO but Linux only has the one domain.
	 */
	case NIOCGETDOMNAM:
		return socksys_getdomainname(arg);
	case NIOCSETDOMNAM:
		return socksys_setdomainname(arg);
	case NIOCLSTAT:
		return socksys_lstat(arg);
	case NIOCOLDGETFH:
	case NIOCGETFH:
		return socksys_getfh(arg);
	case NIOCNFSD:
	case NIOCASYNCD:
	case NIOCCLNTHAND:
	case NIOCEXPORTFS:
		return -EINVAL;

	case SSYS_SIOCSOCKSYS:		/* Pseudo socket syscall */
	case SVR4_SIOCSOCKSYS:
		return socksys_syscall((u_int *)arg);

	case SSYS_SIOCSHIWAT:		/* set high watermark */
	case SVR4_SIOCSHIWAT:
	case SSYS_SIOCSLOWAT:		/* set low watermark */
	case SVR4_SIOCSLOWAT:
		/*
		 * Linux doesn't support them but lie anyway
		 * or some things take it as fatal (why?)
		 *
		 * FIXME: actually we can do this now...
		 */
		return 0;
	case SSYS_SIOCGHIWAT:		/* get high watermark */
	case SVR4_SIOCGHIWAT:
	case SSYS_SIOCGLOWAT:		/* get low watermark */
	case SVR4_SIOCGLOWAT:
		/*
		 * Linux doesn't support them but lie anyway
		 * or some things take it as fatal (why?)
		 *
		 * FIXME: actually we can do this now...
		 */
		if (!access_ok(VERIFY_WRITE, arg, sizeof(u_int)))
			return -EFAULT;
		put_user(0, (u_int *)arg);
		return 0;
	case SSYS_SIOCATMARK:		/* at oob mark? */
	case SVR4_SIOCATMARK:
		error=SYS(ioctl,fd, SIOCATMARK, (long)arg); return error;

	case SSYS_SIOCSPGRP:		/* set process group */
	case SVR4_SIOCSPGRP:
		error=SYS(ioctl,fd, SIOCSPGRP, (long)arg); return error;
	case SSYS_SIOCGPGRP:		/* get process group */
	case SVR4_SIOCGPGRP:
		error=SYS(ioctl,fd, SIOCGPGRP, (long)arg); return error;

	case FIONREAD:
	case SSYS_FIONREAD:		/* BSD compatibilty */
		error = SYS(ioctl,fd, TIOCINQ, (long)arg);
#if defined(CONFIG_ABI_TRACE)
		if (!error && abi_traced(ABI_TRACE_SOCKSYS)) {
			u_long n;

			get_user(n, (u_long *)arg);
			__abi_trace("socksys: %d FIONREAD "
					"found %lu bytes ready\n",
					fd, n);
		}
#endif
		return error;
	case SSYS_FIONBIO: 		/* BSD compatibilty */
		error=SYS(ioctl,fd, FIONBIO, (long)arg); return error;
	case SSYS_FIOASYNC: 		/* BSD compatibilty */
		error=SYS(ioctl,fd, FIOASYNC, (long)arg); return error;
	case SSYS_SIOCADDRT:		/* add route */
	case SVR4_SIOCADDRT:
		error=SYS(ioctl,fd, SIOCADDRT, (long)arg); return error;
	case SSYS_SIOCDELRT:		/* delete route */
	case SVR4_SIOCDELRT:
		error=SYS(ioctl,fd, SIOCDELRT, (long)arg); return error;
	case SSYS_SIOCSIFADDR:		/* set ifnet address */
	case SVR4_SIOCSIFADDR:
		error=SYS(ioctl,fd, SIOCSIFADDR, (long)arg); return error;
	case SSYS_SIOCGIFADDR:		/* get ifnet address */
	case SVR4_SIOCGIFADDR:
		error=SYS(ioctl,fd, SIOCGIFADDR, (long)arg); return error;
	case SSYS_SIOCSIFDSTADDR:	/* set p-p address */
	case SVR4_SIOCSIFDSTADDR:
		error=SYS(ioctl,fd, SIOCSIFDSTADDR, (long)arg); return error;
	case SSYS_SIOCGIFDSTADDR:	/* get p-p address */
	case SVR4_SIOCGIFDSTADDR:
		error=SYS(ioctl,fd, SIOCGIFDSTADDR, (long)arg); return error;
	case SSYS_SIOCSIFFLAGS:		/* set ifnet flags */
	case SVR4_SIOCSIFFLAGS:
		error=SYS(ioctl,fd, SIOCSIFFLAGS, (long)arg); return error;
	case SSYS_SIOCGIFFLAGS:		/* get ifnet flags */
	case SVR4_SIOCGIFFLAGS:
#if 0
	case SVRX_SIOCGIFFLAGS:
#endif
		error=SYS(ioctl,fd, SIOCGIFFLAGS, (long)arg); return error;
	case SSYS_SIOCGIFCONF:		/* get ifnet list */
	case SVR4_SIOCGIFCONF:
#if 0
	case SVRX_SIOCGIFCONF:
#endif
		error=SYS(ioctl,fd, SIOCGIFCONF, (long)arg); return error;
	case SSYS_SIOCGIFBRDADDR:	/* get broadcast addr */
	case SVR4_SIOCGIFBRDADDR:
		error=SYS(ioctl,fd, SIOCGIFBRDADDR, (long)arg); return error;
	case SSYS_SIOCSIFBRDADDR:	/* set broadcast addr */
	case SVR4_SIOCSIFBRDADDR:
		error=SYS(ioctl,fd, SIOCSIFBRDADDR, (long)arg); return error;
	case SSYS_SIOCGIFNETMASK:	/* get net addr mask */
	case SVR4_SIOCGIFNETMASK:
		error=SYS(ioctl,fd, SIOCGIFNETMASK, (long)arg); return error;
	case SSYS_SIOCSIFNETMASK:	/* set net addr mask */
		error=SYS(ioctl,fd, SIOCSIFNETMASK, (long)arg); return error;
	case SSYS_SIOCGIFMETRIC:	/* get IF metric */
	case SVR4_SIOCGIFMETRIC:
		error=SYS(ioctl,fd, SIOCGIFMETRIC, (long)arg); return error;
	case SSYS_SIOCSIFMETRIC:	/* set IF metric */
	case SVR4_SIOCSIFMETRIC:
		error=SYS(ioctl,fd, SIOCSIFMETRIC, (long)arg); return error;
	case SSYS_SIOCSARP:		/* set arp entry */
	case SVR4_SIOCSARP:
		error=SYS(ioctl,fd, SIOCSARP, (long)arg); return error;
	case SSYS_SIOCGARP:		/* get arp entry */
	case SVR4_SIOCGARP:
		error=SYS(ioctl,fd, SIOCGARP, (long)arg); return error;
	case SSYS_SIOCDARP:		/* delete arp entry */
	case SVR4_SIOCDARP:
		error=SYS(ioctl,fd, SIOCDARP, (long)arg); return error;
	case SSYS_SIOCGENADDR:		/* Get ethernet addr */
	case SVR4_SIOCGENADDR:
		error=SYS(ioctl,fd, SIOCGIFHWADDR, (long)arg); return error;
	case SSYS_SIOCSIFMTU:		/* get if_mtu */
	case SVR4_SIOCSIFMTU:
		error=SYS(ioctl,fd, SIOCSIFMTU, (long)arg); return error;
	case SSYS_SIOCGIFMTU:		/* set if_mtu */
	case SVR4_SIOCGIFMTU:
		error=SYS(ioctl,fd, SIOCGIFMTU, (long)arg); return error;

	case SSYS_SIOCGETNAME:		/* getsockname */
	case SVR4_SIOCGETNAME:
		return socksys_getsockname(fd, arg);
	case SSYS_SIOCGETPEER: 		/* getpeername */
	case SVR4_SIOCGETPEER:
		return socksys_getpeername(fd, arg);

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

	case SSYS_SIOCPROTO:		/* link proto */
	case SVR4_SIOCPROTO:
	case SSYS_SIOCX25XMT:
	case SVR4_SIOCX25XMT:
	case SSYS_SIOCX25RCV:
	case SVR4_SIOCX25RCV:
	case SSYS_SIOCX25TBL:
	case SVR4_SIOCX25TBL:

	case SSYS_SIOCGIFONEP:		/* get one-packet params */
		return socksys_gifonep(arg);
	case SSYS_SIOCSIFONEP:		/* set one-packet params */
		return socksys_sifonep(arg);

	default:
		printk(KERN_DEBUG "%d iBCS: socksys: %d: ioctl 0x%x with argument 0x%p requested\n",
			current->pid, fd, cmd, arg);
		break;
	}

	return -EINVAL;
}
extern void svr4_initmap(void);
extern void svr4_freemap(void);

static int __init
socksys_init(void)
{
	int ret;

	if ((ret = register_chrdev(SOCKSYS_MAJOR, "socksys", &socksys_fops))) {
		printk(KERN_ERR "abi: unable register socksys char major\n");
		return (ret);
	}

/*	fops_get(&socket_file_ops);
	socksys_socket_fops = socket_file_ops;
#ifdef CONFIG_ABI_XTI
	socksys_socket_fops.release = socksys_release;
	socksys_socket_fops.poll = socksys_poll;
#endif
*/
	svr4_initmap();
	return (0);
}

static void __exit
socksys_exit(void)
{
/*	fops_put(&socket_file_ops);*/
	svr4_freemap();
	unregister_chrdev(SOCKSYS_MAJOR, "socksys");
}

module_init(socksys_init);
module_exit(socksys_exit);

EXPORT_SYMBOL(abi_ioctl_socksys);
EXPORT_SYMBOL(socksys_syscall);
