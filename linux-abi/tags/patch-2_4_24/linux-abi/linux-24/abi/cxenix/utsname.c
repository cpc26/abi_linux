/*
 * utsname.c - support for the utsname subcall of cxenix
 *
 * Copyright (C) 1994 Mike Jagdis (jaggy@purplet.demon.co.uk)
 */

#ident "%W% %G%"

#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/personality.h>
#include <linux/utsname.h>
#include <asm/uaccess.h>


static char	sco_serial[10] = "public";

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

#define set_utsfield(to, from, dotchop) \
	{ \
		char *p; \
		int i, len = (sizeof(to) > sizeof(from) ? sizeof(from) : sizeof(to)); \
		__copy_to_user(to, from, len); \
		if (dotchop) \
			for (p=from,i=0; *p && *p != '.' && --len; p++,i++); \
		else \
			i = len - 1; \
		__put_user('\0', to+i); \
	}


int
xnx_utsname(u_long addr)
{
	struct xnx_utsname	*utp = (struct xnx_utsname *)addr;
	int			error;

	/*
	 * This shouldn't be invoked by anything that isn't running
	 * in the SCO personality. I can envisage a program that uses
	 * this to test if utp is running on SCO or not. It probably
	 * won't happen but let's make sure utp doesn't anyway...
	 */
	if (!personality(PER_SCOSVR3))
		return -ENOSYS;

	down_read(&uts_sem);
	error = verify_area(VERIFY_WRITE, utp, sizeof (struct xnx_utsname));
	if (!error) {
		if (abi_fake_utsname) {
			set_utsfield(utp->sysname, "SCO_SV", 0);
			set_utsfield(utp->nodename, system_utsname.nodename, 1);
			set_utsfield(utp->release, "3.2v5.0.0\0", 0);
		} else {
			set_utsfield(utp->sysname, system_utsname.nodename, 1);
			set_utsfield(utp->nodename, system_utsname.nodename, 1);
			set_utsfield(utp->release, system_utsname.release, 0);
		}
		set_utsfield(utp->kernelid, system_utsname.version, 0);
		set_utsfield(utp->machine, system_utsname.machine, 0);
		if (EISA_bus) {
			set_utsfield(utp->bustype, "EISA", 0);
		} else {
			set_utsfield(utp->bustype, "ISA", 0);
		}
		set_utsfield(utp->sysserial, sco_serial, 0);
		__put_user(0xffff, &utp->sysorigin);
		__put_user(0xffff, &utp->sysoem);
		set_utsfield(utp->numusers, "unlim", 0);
		__put_user(1, &utp->numcpu);
	}
	up_read(&uts_sem);

	return (error);
}
