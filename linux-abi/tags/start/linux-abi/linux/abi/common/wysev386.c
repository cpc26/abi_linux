/*
 *  linux/ibcs/wysev386.c
 *
 *  Copyright 1994, 1996  Mike Jagdis (jaggy@purplet.demon.co.uk)
 *
 * $Id$
 * $Source$
 */

#include <linux/config.h>
#define __NO_VERSION__
#include <linux/module.h>

#include <linux/version.h>

#include <asm/uaccess.h>

#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/utsname.h>

#include <linux/wait.h>

#include <linux/net.h>
#include <linux/sys.h>

#include <abi/abi.h>
#include <abi/map.h>
#include <abi/socket.h>


int abi_gethostname(char *name, int len)
{
	int	error;
	char	*p;

	down_read(&uts_sem);
	error = verify_area(VERIFY_WRITE, name, len);
	if (!error) {
		--len;
		for (p=system_utsname.nodename; *p && len; p++,len--) {
			__put_user(*p, name);
			name++;
		}
		__put_user('\0', name);
	}
	up_read(&uts_sem);

	return error;
}

EXPORT_SYMBOL(abi_gethostname);

int abi_getdomainname(char *name, int len)
{
	int	error;
	char	*p;

	down_read(&uts_sem);
	error = verify_area(VERIFY_WRITE, name, len);
	if (!error) {
		--len;
		for (p=system_utsname.domainname; *p && len; p++,len--) {
			__put_user(*p, name);
			name++;
		}
		__put_user('\0', name);
	}
	up_read(&uts_sem);

	return error;
}

EXPORT_SYMBOL(abi_getdomainname);

int abi_wait3(int *loc)
{
	int pid;

	pid = SYS(wait4)(-1, loc, WNOHANG, 0);

	if(loc) {
		int res;
		__get_user(res, (unsigned long *) loc);
		if ((res & 0xff) == 0x7f) {
			int sig = (res >> 8) & 0xff;
			sig = current->exec_domain->signal_map[sig];
			res = (res & (~0xff00)) | (sig << 8);
			put_user(res, (unsigned long *)loc);
		} else if (res && res == (res & 0xff)) {
			res = current->exec_domain->signal_map[res & 0x7f];
			put_user(res, (unsigned long *)loc);
		}
	}

	return pid;
}

EXPORT_SYMBOL(abi_wait3);


/* It would probably be better to remove the statics in linux/net/socket.c
 * and go direct to the sock_ calls than via the indirection routine.
 */
int abi_socket(struct pt_regs *regs)
{
	unsigned long v;

	get_user(v, ((unsigned long*)regs->esp)+1);
	put_user(
		map_value(current->exec_domain->af_map, v, 0),
		((unsigned long *)regs->esp)+1);
	get_user(v, ((unsigned long*)regs->esp)+2);
	put_user(
		map_value(current->exec_domain->socktype_map, v, 0),
		((unsigned long *)regs->esp)+2);

	return SYS(socketcall)(SYS_SOCKET, ((unsigned long *)regs->esp) + 1);
}

EXPORT_SYMBOL(abi_socket);

int abi_connect(struct pt_regs *regs)
{
	return SYS(socketcall)(SYS_CONNECT, ((unsigned long *)regs->esp) + 1);
}

EXPORT_SYMBOL(abi_connect);

int abi_accept(struct pt_regs *regs)
{
	return SYS(socketcall)(SYS_ACCEPT, ((unsigned long *)regs->esp) + 1);
}

EXPORT_SYMBOL(abi_accept);

int abi_send(struct pt_regs *regs)
{
	int err = SYS(socketcall)(SYS_SEND, ((unsigned long *)regs->esp) + 1);
	if (err == -EAGAIN) err = -EWOULDBLOCK;
	return err;
}

EXPORT_SYMBOL(abi_send);

int abi_recv(struct pt_regs *regs)
{
	int err = SYS(socketcall)(SYS_RECV, ((unsigned long *)regs->esp) + 1);
	if (err == -EAGAIN) err = -EWOULDBLOCK;
	return err;
}

EXPORT_SYMBOL(abi_recv);


int abi_bind(struct pt_regs *regs)
{
	return SYS(socketcall)(SYS_BIND, ((unsigned long *)regs->esp) + 1);
}

EXPORT_SYMBOL(abi_bind);

int abi_setsockopt(struct pt_regs *regs)
{
	return abi_do_setsockopt(((unsigned long *)regs->esp) + 1);
}

EXPORT_SYMBOL(abi_setsockopt);

int abi_listen(struct pt_regs *regs)
{
	return SYS(socketcall)(SYS_LISTEN, ((unsigned long *)regs->esp) + 1);
}

EXPORT_SYMBOL(abi_listen);

int abi_getsockopt(struct pt_regs *regs)
{
	return abi_do_getsockopt(((unsigned long *)regs->esp) + 1);
}

EXPORT_SYMBOL(abi_getsockopt);

int abi_recvfrom(struct pt_regs *regs)
{
	int err = SYS(socketcall)(SYS_RECVFROM, ((unsigned long *)regs->esp) + 1);
	if (err == -EAGAIN) err = -EWOULDBLOCK;
	return err;
}

EXPORT_SYMBOL(abi_recvfrom);

int abi_sendto(struct pt_regs *regs)
{
	int err = SYS(socketcall)(SYS_SENDTO, ((unsigned long *)regs->esp) + 1);
	if (err == -EAGAIN) err = -EWOULDBLOCK;
	return err;
}

EXPORT_SYMBOL(abi_sendto);

int abi_shutdown(struct pt_regs *regs)
{
	return SYS(socketcall)(SYS_SHUTDOWN, ((unsigned long *)regs->esp) + 1);
}

EXPORT_SYMBOL(abi_shutdown);

int abi_socketpair(struct pt_regs *regs)
{
	return SYS(socketcall)(SYS_SOCKETPAIR, ((unsigned long *)regs->esp) + 1);
}

EXPORT_SYMBOL(abi_socketpair);

int abi_getpeername(struct pt_regs *regs)
{
	return SYS(socketcall)(SYS_GETPEERNAME, ((unsigned long *)regs->esp) + 1);
}

EXPORT_SYMBOL(abi_getpeername);

int abi_getsockname(struct pt_regs *regs)
{
	return SYS(socketcall)(SYS_GETSOCKNAME, ((unsigned long *)regs->esp) + 1);
}

EXPORT_SYMBOL(abi_getsockname);
