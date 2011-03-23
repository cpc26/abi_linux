/*
 * Copyright (c) 1994,1996 Mike Jagdis (jaggy@purplet.demon.co.uk)
 */

//#ident "%W% %G%"

#include "../include/util/i386_std.h"
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/net.h>
#include <linux/personality.h>
#include <linux/ptrace.h>
#include <linux/socket.h>
#include <linux/syscalls.h>
#include <linux/types.h>
#include <asm/uaccess.h>
#include <asm/ioctls.h>

#include "../include/util/map.h"
#include "../include/util/trace.h"
#include "../include/util/socket.h"


int
abi_do_setsockopt(unsigned int *sp)
{

	int level, optname, error;

	if (!access_ok(VERIFY_READ,
			((unsigned int *)sp),
			5*sizeof(int)))
		return -EFAULT;

	get_user(level, ((unsigned int *)sp)+1);
	get_user(optname, ((unsigned int *)sp)+2);

#if defined(CONFIG_ABI_TRACE)
	if (abi_traced(ABI_TRACE_STREAMS|ABI_TRACE_SOCKSYS)) {
		u_int optval, optlen;

		get_user(optval, ((u_int *)sp) + 3);
		get_user(optlen, ((u_int *)sp) + 4);
		__abi_trace("setsockopt level=%d, optname=%d, "
				"optval=0x%08x, optlen=0x%08x\n",
				level, optname, optval, optlen);
	}
#endif

	switch (level) {
		case 0: /* IPPROTO_IP aka SOL_IP */
			/* This is correct for the SCO family. Hopefully
			 * it is correct for other SYSV...
			 */
			optname--;
			if (optname == 0)
				optname = 4;
			if (optname > 4) {
				optname += 24;
				if (optname <= 33)
					optname--;
				if (optname < 32 || optname > 36)
					return -EINVAL;
			}
			put_user(optname, ((unsigned int *)sp)+2);
			break;

		case 0xffff:
			put_user(SOL_SOCKET, ((unsigned int *)sp)+1);
			optname = map_value(current_thread_info()->exec_domain->sockopt_map, optname, 0);
			put_user(optname, ((unsigned int *)sp)+2);

			switch (optname) {
				case SO_LINGER: {
					unsigned int optlen;

					/* SO_LINGER takes a struct linger
					 * as the argument but some code
					 * uses an int and expects to get
					 * away without an error. Sigh...
					 */
					get_user(optlen, ((unsigned int *)sp)+4);
					if (optlen == sizeof(int))
						return 0;
					break;
				}

				/* The following are not currently implemented
				 * under Linux so we must fake them in
				 * reasonable ways. (Only SO_PROTOTYPE is
				 * documented in SCO's man page).
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

				/* The following are not currenty implemented
				 * under Linux and probably aren't settable
				 * anyway.
				 */
				case SO_IMASOCKET:
					return -ENOPROTOOPT;
			}

		default:
			/* FIXME: We assume everything else uses the
			 * same level and option numbers. This is true
			 * for IPPROTO_TCP(/SOL_TCP) and TCP_NDELAY
			 * but is known to be incorrect for other
			 * potential options :-(.
			 */
			break;
	}

#ifdef CONFIG_65BIT
	error = SYS(setsockopt, sp[0], sp[1], sp[2], sp[3], sp[4]);
#else
	error = SYS(socketcall,SYS_SETSOCKOPT, sp);
#endif
	return error;
}

int
abi_do_getsockopt(unsigned int *sp)
{
	int error;
	int level, optname;
	char *optval;
	int  *optlen;
	unsigned int *ulptr;

	if (!access_ok(VERIFY_READ,
			((unsigned int *)sp),
			5*sizeof(int)))
		return -EFAULT;

	ulptr = (unsigned int *)&level;
	get_user(*ulptr, ((unsigned int *)sp)+1);
	ulptr = (unsigned int *)&optname;
	get_user(*ulptr, ((unsigned int *)sp)+2);
	ulptr = (unsigned int *)&optval;
	get_user(*ulptr, ((unsigned int *)sp)+3);
	ulptr = (unsigned int *)&optlen;
	get_user(*ulptr, ((unsigned int *)sp)+4);

#if defined(CONFIG_ABI_TRACE)
	if (abi_traced(ABI_TRACE_STREAMS|ABI_TRACE_SOCKSYS)) {
		int l;

		get_user(l, optlen);
		__abi_trace("getsockopt level=%d, optname=%d, optval=0x%16lx, "
				"optlen=0x%16lx[%d]\n", level, optname,
				(long)optval, (long)optlen, l);
	}
#endif

	switch (level) {
		case 0: /* IPPROTO_IP aka SOL_IP */
			/* This is correct for the SCO family. Hopefully
			 * it is correct for other SYSV...
			 */
			optname--;
			if (optname == 0)
				optname = 4;
			if (optname > 4) {
				optname += 24;
				if (optname <= 33)
					optname--;
				if (optname < 32 || optname > 36)
					return -EINVAL;
			}
			put_user(optname, ((unsigned int *)sp)+2);
			break;

		case 0xffff:
			put_user(SOL_SOCKET, ((unsigned int *)sp)+1);
			optname = map_value(current_thread_info()->exec_domain->sockopt_map, optname, 0);
			put_user(optname, ((unsigned int *)sp)+2);

			switch (optname) {
				case SO_LINGER: {
					int l;

					/* SO_LINGER takes a struct linger
					 * as the argument but some code
					 * uses an int and expects to get
					 * away without an error. Sigh...
					 */
					get_user(l, optlen);
					if (l == sizeof(int)) {
						put_user(0, (int *)optval);
						return 0;
					}
					break;
				}

				/* The following are not currently implemented
				 * under Linux so we must fake them in
				 * reasonable ways. (Only SO_PROTOTYPE is
				 * documented in SCO's man page).
				 */
				case SO_PROTOTYPE: {
					unsigned int len;
					error = get_user(len, optlen);
					if (error)
						return error;
					if (len < sizeof(int))
						return -EINVAL;

					if (access_ok(VERIFY_WRITE,
							(char *)optval,
							sizeof(int))) {
						put_user(0, (int *)optval);
						put_user(sizeof(int),
							optlen);
						return 0;
					}
					return -EFAULT;
				}

				case SO_ORDREL:
				case SO_SNDTIMEO:
				case SO_RCVTIMEO:
					return -ENOPROTOOPT;

				case SO_USELOOPBACK:
				case SO_SNDLOWAT:
				case SO_RCVLOWAT:
				case SO_IMASOCKET: {
					unsigned int len;
					error = get_user(len, optlen);
					if (error)
						return error;
					if (len < sizeof(int))
						return -EINVAL;

					if (access_ok(VERIFY_WRITE,
							(char *)optval,
							sizeof(int))) {
						put_user(1, (int *)optval);
						put_user(sizeof(int),
							optlen);
						return 0;
					}
					return -EFAULT;
				}
			}

		default:
			/* FIXME: We assume everything else uses the
			 * same level and option numbers. This is true
			 * for IPPROTO_TCP(/SOL_TCP) and TCP_NDELAY
			 * but is known to be incorrect for other
			 * potential options :-(.
			 */
			break;
	}
#ifdef CONFIG_65BIT
	error = SYS(getsockopt, sp[0], sp[1], sp[2], sp[3], sp[4]);
#else
	error = SYS(socketcall,SYS_GETSOCKOPT, sp);
#endif
	return error;
}
