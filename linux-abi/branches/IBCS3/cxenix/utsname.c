/*
 * utsname.c - support for the utsname subcall of cxenix
 *
 * Copyright (C) 1994 Mike Jagdis (jaggy@purplet.demon.co.uk)
 */

//#ident "%W% %G%"

#include "../include/util/i386_std.h"
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/personality.h>
#include <linux/utsname.h>
#include <linux/eisa.h>
#include <asm/uaccess.h>

static char	sco_serial[10] = "public";
#define abi_fake_utsname 0
#if _KSL > 18
#define system_utsname init_uts_ns.name
#endif
#include "../include/util/trace.h"

struct xnx_utsname {
	char	sysname[9];
	char	nodename[9];
	char	release[16];
	char	kernelid[20];
	char	machine[9];
	char	bustype[9];
	char	sysserial[10];
	u_short	sysorigin;
	u_short	sysoem;
	char	numusers[9];
	u_short	numcpu;
};

/* this function and macro design is from arch/sparc64/solaris/misc.c */
static int __set_utsfield(char __user *to, int to_size,
			const char *from, int from_size,
			int dotchop)
{
	int len = min(from_size, to_size);
	int off; const char *p;

	if (copy_to_user(to, from, len))
		return -EFAULT;

	off = len < to_size? len: len - 1;
	if (dotchop) {
		/*const char *p = strnchr(from, len, '.');
		if (p) off =  p - from;*/
		p=from; while(p[0] != '\0' && p[0] != '.') p++;
		if (p[0]=='.') off =  p - from;
	}

	if (__put_user('\0', to + off))
		return -EFAULT;

	return 0;
}

#define set_utsfield(to, from, dotchop) \
	__set_utsfield((to), sizeof(to), \
		(from), sizeof(from), \
		(dotchop))

static int __set_utsvalue(void __user *to, int to_size,
	const void *from, int from_size)
{
	int len = min(from_size, to_size);

	if (copy_to_user(to, from, len))
		return -EFAULT;

	return 0;
}

#define set_utsvalue(to, from) \
  __set_utsvalue((to), sizeof(*to), \
    (from), sizeof(*from))

int
xnx_utsname(u_long addr)
{
	struct xnx_utsname	*utp = (struct xnx_utsname *)addr;
	unsigned short usvalue;
	long lvalue;
	int error=0;

	/*
	 * This shouldn't be invoked by anything that isn't running
	 * in the SCO personality. I can envisage a program that uses
	 * this to test if utp is running on SCO or not. It probably
	 * won't happen but let's make sure utp doesn't anyway...
	 */
	if (current->personality != PER_SCOSVR3)
		return -ENOSYS;

	down_read(&uts_sem);
	if (access_ok(VERIFY_WRITE, utp, sizeof (struct xnx_utsname))) {
		if (abi_fake_utsname) {
			error |= set_utsfield(utp->sysname, "SCO_SV", 0);
			error |= set_utsfield(utp->nodename, system_utsname.nodename, 1);
			error |= set_utsfield(utp->release, "3.2v5.0.0\0", 0);
		} else {
			error |= set_utsfield(utp->sysname, system_utsname.nodename, 1);
			error |= set_utsfield(utp->nodename, system_utsname.nodename, 1);
			error |= set_utsfield(utp->release, system_utsname.release, 0);
		}
		error |= set_utsfield(utp->kernelid, system_utsname.version, 0);
		error |= set_utsfield(utp->machine, system_utsname.machine, 0);
		if (EISA_bus) {
			error |= set_utsfield(utp->bustype, "EISA", 0);
		} else {
			error |= set_utsfield(utp->bustype, "ISA", 0);
		}
		error |= set_utsfield(utp->sysserial, sco_serial, 0);
		usvalue = 0xffff;
		error |= set_utsvalue(&utp->sysorigin, &usvalue);
		error |= set_utsvalue(&utp->sysoem, &usvalue);
		error |= set_utsfield(utp->numusers, "unlim", 0);
		lvalue = 1;
		error |= set_utsvalue(&utp->numcpu, &lvalue);
	}
	up_read(&uts_sem);

	return (error);
}
