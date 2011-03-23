/*
 * Copyright (c) 1994,1996 Mike Jagdis.
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
 *
 */

//#ident "%W% %G%"

/*
 * BSD-style socket support for Wyse/V386.
 */
#include "../include/util/i386_std.h"
#include <linux/net.h>
#include <linux/personality.h>
#include <linux/syscalls.h>
#include <linux/utsname.h>
#include <linux/signal.h>
#include <linux/wait.h>
#include <linux/socket.h>
#include <linux/net.h>
#include <linux/mm.h>
#include <asm/uaccess.h>

#include "../include/abi_reg.h"
#include "../include/util/map.h"
#include "../include/util/socket.h"
#include "../include/wyse/sysent.h"
#include "../include/util/trace.h"
#if _KSL > 18
#define system_utsname init_uts_ns.name
#endif


int wyse_gethostname(char *name, int len)
{
	int error = 0;
	char *p;

	down_read(&uts_sem);
	if (!access_ok(VERIFY_WRITE, name, len)) {
		error = -EFAULT;
		--len;
		for (p = system_utsname.nodename; *p && len; p++,len--) {
			__put_user(*p, name);
			name++;
		}
		__put_user('\0', name);
	}
	up_read(&uts_sem);
	return error;
}

int wyse_getdomainname(char *name, int len)
{
	int error;
	char *p;

	down_read(&uts_sem);
	error = 0;
	if (!access_ok(VERIFY_WRITE, name, len)) {
		error = -EFAULT;
		--len;
		for (p = system_utsname.domainname; *p && len; p++,len--) {
			__put_user(*p, name);
			name++;
		}
		__put_user('\0', name);
	}
	up_read(&uts_sem);
	return error;
}

int wyse_wait3(int *loc)
{
	pid_t pid;
	int res;

	pid = SYS(wait4,-1, loc, WNOHANG, 0);
	if (!loc)
		return pid;

	get_user(res, (unsigned long *)loc);

	if ((res & 0xff) == 0x7f) {
		int sig = (res >> 8) & 0xff;

		sig = current_thread_info()->exec_domain->signal_map[sig];
		res = (res & (~0xff00)) | (sig << 8);
		put_user(res, (unsigned long *)loc);
	} else if (res && res == (res & 0xff)) {
		res = current_thread_info()->exec_domain->signal_map[res & 0x7f];
		put_user(res, (unsigned long *)loc);
	}

	return pid;
}

int wyse_socket(int family, int type, int protocol)
{
int ret, a[3]; mm_segment_t fs;
	family = map_value(current_thread_info()->exec_domain->af_map, family, 0);
	type = map_value(current_thread_info()->exec_domain->socktype_map, family, 0);

	fs=get_fs(); set_fs(get_ds());
	a[0]=family; a[1]=type; a[2]=protocol;
#ifdef CONFIG_65BIT
	ret = SYS(socket,family, type, protocol);
#else
	ret = SYS(socketcall,SYS_SOCKET,a);
#endif
	set_fs(fs); return ret;
}

int wyse_setsockopt(int fd, int level, int optname, char *optval, int optlen)
{
int ret, a[5]; mm_segment_t fs;
	switch (level) {
	case 0: /* IPPROTO_IP aka SOL_IP */
		if (--optname == 0)
			optname = 4;
		if (optname > 4) {
			optname += 24;
			if (optname <= 33)
				optname--;
			if (optname < 32 || optname > 36)
				return -EINVAL;
			break;
		}
	case 0xffff:
		level = SOL_SOCKET;
		optname = map_value(current_thread_info()->exec_domain->sockopt_map, optname, 0);
		switch (optname) {
		case SO_LINGER:
			/*
			 * SO_LINGER takes a struct linger as the argument
			 * but some code uses an int and expects to get
			 * away without an error. Sigh...
			 */
			if (optlen == sizeof(int))
				return 0;
			break;
		/*
		 * The following are not currently implemented under Linux
		 * so we must fake them in reasonable ways.
		 * (Only SO_PROTOTYPE is documented in SCO's man page).
		 */
		case SO_PROTOTYPE:
		case SO_ORDREL:
		case SO_SNDTIMEO:
		case SO_RCVTIMEO:
			return -ENOPROTOOPT;

		case SO_USELOOPBACK:
		case SO_SNDLOWAT:
		case SO_RCVLOWAT:
			return 0;

		/*
		 * The following are not currenty implemented under Linux
		 * and probably aren't settable anyway.
		 */
		case SO_IMASOCKET:
			return -ENOPROTOOPT;
		default:
			break;
		}
	default:
		/*
		 * XXX We assume everything else uses the same level and
		 * XXX option numbers.  This is true for IPPROTO_TCP(/SOL_TCP)
		 * XXX and TCP_NDELAY but is known to be incorrect for other
		 * XXX potential options :-(.
		 */
		break;
	}

	fs=get_fs(); set_fs(get_ds());
	a[0]=fd; a[1]=level; a[2]=optname; a[3]=(int)(long)optval; a[4]=optlen;
#ifdef CONFIG_65BIT
	ret = SYS(setsockopt,fd, level, optname, optval, optlen);
#else
	ret = SYS(socketcall,SYS_SETSOCKOPT,a);
#endif
	set_fs(fs); return ret;
}

int wyse_getsockopt(int fd, int level, int optname, char *optval, int *optlen)
{
int ret, a[5]; mm_segment_t fs;
	unsigned int len;
	int val;

	if (get_user(len,optlen))
		return -EFAULT;
	if (len < 0)
		return -EINVAL;

	switch (level) {
	case 0: /* IPPROTO_IP aka SOL_IP */
		if (--optname == 0)
			optname = 4;
		if (optname > 4) {
			optname += 24;
			if (optname <= 33)
				optname--;
			if (optname < 32 || optname > 36)
				return -EINVAL;
			break;
		}
	case 0xffff:
		level = SOL_SOCKET;
		optname = map_value(current_thread_info()->exec_domain->sockopt_map, optname, 0);
		switch (optname) {
		case SO_LINGER:
			/*
			 * SO_LINGER takes a struct linger as the argument
			 * but some code uses an int and expects to get
			 * away without an error. Sigh...
			 */
			if (len != sizeof(int))
				goto native;

			val = 0;
			break;
		/*
		 * The following are not currently implemented under Linux
		 * so we must fake them in reasonable ways.
		 * (Only SO_PROTOTYPE is documented in SCO's man page).
		 */
		case SO_PROTOTYPE:
			val = 0;
			break;

		case SO_ORDREL:
		case SO_SNDTIMEO:
		case SO_RCVTIMEO:
			return -ENOPROTOOPT;

		case SO_USELOOPBACK:
		case SO_SNDLOWAT:
		case SO_RCVLOWAT:
			return 0;

		/*
		 * The following are not currenty implemented under Linux
		 * and probably aren't settable anyway.
		 */
		case SO_IMASOCKET:
			val = 1;
			break;
		default:
			goto native;
		}

		if (len > sizeof(int))
			len = sizeof(int);
		if (copy_to_user(optval, &val, len))
			return -EFAULT;
		if (put_user(len, optlen))
			 return -EFAULT;
		return 0;

	default:
		/*
		 * XXX We assume everything else uses the same level and
		 * XXX option numbers.  This is true for IPPROTO_TCP(/SOL_TCP)
		 * XXX and TCP_NDELAY but is known to be incorrect for other
		 * XXX potential options :-(.
		 */
		break;
	}

native:
	fs=get_fs(); set_fs(get_ds());
	a[0]=fd; a[1]=level; a[2]=optname; a[3]=(int)(long)optval; a[4]=(int)(long)optlen;
#ifdef CONFIG_65BIT
	ret = SYS(getsockopt,fd, level, optname, optval, optlen);
#else
	ret = SYS(socketcall,SYS_GETSOCKOPT,a);
#endif
	set_fs(fs); return ret;
}

int wyse_recvfrom(int fd, void *buff, size_t size, unsigned flags,
		  struct sockaddr *addr, int *addr_len)
{
	int error;
int a[6]; mm_segment_t fs;

	fs=get_fs(); set_fs(get_ds());
	a[0]=fd; a[1]=(int)(long)buff; a[2]=size; a[3]=flags;
	a[4]=(int)(long)addr; a[5]=(int)(long)addr_len;
#ifdef CONFIG_65BIT
	error = SYS(recvfrom,fd, buff, size, flags, addr, addr_len);
#else
	error = SYS(socketcall,SYS_RECVFROM,a);
#endif
	set_fs(fs);
	if (error == -EAGAIN)
		error = -EWOULDBLOCK;
	return error;
}

int wyse_recv(int fd, void *buff, size_t size, unsigned flags)
{
	int error;
int a[6]; mm_segment_t fs;

	fs=get_fs(); set_fs(get_ds());
	a[0]=fd; a[1]=(int)(long)buff; a[2]=size; a[3]=flags; a[4]=0; a[5]=0;
#ifdef CONFIG_65BIT
	error = SYS(recvfrom,fd, buff, size, flags, NULL, NULL);
#else
	error = SYS(socketcall,SYS_RECVFROM,a);
#endif
	set_fs(fs);
	if (error == -EAGAIN)
		error = -EWOULDBLOCK;
	return error;
}

int wyse_sendto(int fd, void *buff, size_t len, unsigned flags,
		struct sockaddr *addr, int addr_len)
{
	int error;
int a[6]; mm_segment_t fs;

	fs=get_fs(); set_fs(get_ds());
	a[0]=fd; a[1]=(int)(long)buff; a[2]=len;
	a[3]=flags; a[4]=(int)(long)addr; a[5]=(int)(long)addr_len;
#ifdef CONFIG_65BIT
	error = SYS(sendto,fd, buff, len, flags, addr, addr_len);
#else
	error = SYS(socketcall,SYS_SENDTO,a);
#endif
	set_fs(fs);
	if (error == -EAGAIN)
		error = -EWOULDBLOCK;
	return error;
}

int wyse_send(int fd, void *buff, size_t len, unsigned flags)
{
	int error;
int a[6]; mm_segment_t fs;

	fs=get_fs(); set_fs(get_ds());
	a[0]=fd; a[1]=(int)(long)buff; a[2]=len; a[3]=flags; a[4]=0; a[5]=0;
#ifdef CONFIG_65BIT
	error = SYS(sendto,fd, buff, len, flags, NULL, 0);
#else
	error = SYS(socketcall,SYS_SENDTO,a);
#endif
	set_fs(fs);
	if (error == -EAGAIN)
		error = -EWOULDBLOCK;
	return error;
}

int wyse_connect(struct pt_regs *regs)
{
	int ret;
#ifdef CONFIG_65BIT
	ret = SYS(connect, _BX(regs), _CX(regs), _DX(regs));
#else
	ret = SYS(socketcall,SYS_CONNECT,((unsigned long *)_SP(regs)) + 1);
#endif
	return ret;
}

int wyse_accept(struct pt_regs *regs)
{
	int ret;
#ifdef CONFIG_65BIT
	ret = SYS(accept, _BX(regs), _CX(regs), _DX(regs));
#else
	ret = SYS(socketcall,SYS_ACCEPT,((unsigned long *)_SP(regs)) + 1);
#endif
	return ret;
}

int wyse_bind(struct pt_regs *regs)
{
	int ret;
#ifdef CONFIG_65BIT
	ret = SYS(bind, _BX(regs), _CX(regs), _DX(regs));
#else
	ret = SYS(socketcall,SYS_BIND,((unsigned long *)_SP(regs)) + 1);
#endif
	return ret;
}

int wyse_listen(struct pt_regs *regs)
{
	int ret;
#ifdef CONFIG_65BIT
	ret = SYS(listen, _BX(regs), _CX(regs));
#else
	ret = SYS(socketcall,SYS_LISTEN,((unsigned long *)_SP(regs)) + 1);
#endif
	return ret;
}

int wyse_shutdown(struct pt_regs *regs)
{
	int ret;
#ifdef CONFIG_65BIT
	ret = SYS(shutdown, _BX(regs), _CX(regs));
#else
	ret = SYS(socketcall,SYS_SHUTDOWN,((unsigned long *)_SP(regs)) + 1);
#endif
	return ret;
}

int wyse_socketpair(struct pt_regs *regs)
{
	int ret;
#ifdef CONFIG_65BIT
	ret = SYS(socketpair, _BX(regs), _CX(regs), _DX(regs), _SI(regs));
#else
	ret = SYS(socketcall,SYS_SOCKETPAIR,((unsigned long *)_SP(regs)) + 1);
#endif
	return ret;
}

int wyse_getpeername(struct pt_regs *regs)
{
	int ret;
#ifdef CONFIG_65BIT
	ret = SYS(getpeername, _BX(regs), _CX(regs), _DX(regs));
#else
	ret = SYS(socketcall,SYS_GETPEERNAME,((unsigned long *)_SP(regs)) + 1);
#endif
	return ret;
}

int wyse_getsockname(struct pt_regs *regs)
{
	int ret;
#ifdef CONFIG_65BIT
	ret = SYS(getsockname, _BX(regs), _CX(regs), _DX(regs));
#else
	ret = SYS(socketcall,SYS_GETSOCKNAME,((unsigned long *)_SP(regs)) + 1);
#endif
	return ret;
}
