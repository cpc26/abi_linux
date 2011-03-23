/*
 *  abi/svr4_common/utsname.c
 *
 *  Copyright (C) 1994 Mike Jagdis (jaggy@purplet.demon.co.uk)
 *
 *  The SVR4 utsname support is based on the code originally in svr4.c
 *  which was:
 *
 *  Copyright (C) 1994 Eric Youngdale.
 */

#include <linux/config.h>
#define __NO_VERSION__
#include <linux/module.h>
#include <linux/version.h>

#include <asm/uaccess.h>

#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/personality.h>
#include <linux/utsname.h>

#include <abi/abi.h>

char sco_serial[10] = "public";

struct sco_utsname {
	char sysname[9];
	char nodename[9];
	char release[16];
	char kernelid[20];
	char machine[9];
	char bustype[9];
	char sysserial[10];
	unsigned short sysorigin;
	unsigned short sysoem;
	char numusers[9];
	unsigned short numcpu;
};

struct v7_utsname {
	char sysname[9];
	char nodename[9];
	char release[9];
	char version[9];
	char machine[9];
};

#define SVR4_NMLN 257
struct svr4_utsname {
	char sysname[SVR4_NMLN];
	char nodename[SVR4_NMLN];
	char release[SVR4_NMLN];
	char version[SVR4_NMLN];
	char machine[SVR4_NMLN];
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
sco_utsname(unsigned long addr)
{
	int error;
	struct sco_utsname *it = (struct sco_utsname *)addr;

	/* This shouldn't be invoked by anything that isn't running
	 * in the SCO personality. I can envisage a program that uses
	 * this to test if it is running on SCO or not. It probably
	 * won't happen but let's make sure it doesn't anyway...
	 */
	if (!personality(PER_SCOSVR3))
		return -ENOSYS;

	down_read(&uts_sem);
	error = verify_area(VERIFY_WRITE, it, sizeof (struct sco_utsname));
	if (!error) {
#if 0
		set_utsfield(it->sysname, system_utsname.nodename, 1);
#else
		set_utsfield(it->sysname, "SCO_SV", 0);
#endif
		set_utsfield(it->nodename, system_utsname.nodename, 1);
#if 0
		set_utsfield(it->release, system_utsname.release, 0);
#else
		set_utsfield(it->release, "3.2v5.0.0\0", 0);
#endif
		set_utsfield(it->kernelid, system_utsname.version, 0);
		set_utsfield(it->machine, system_utsname.machine, 0);
		if (EISA_bus) {
			set_utsfield(it->bustype, "EISA", 0);
		} else {
			set_utsfield(it->bustype, "ISA", 0);
		}
		set_utsfield(it->sysserial, sco_serial, 0);
		__put_user(0xffff, &it->sysorigin);
		__put_user(0xffff, &it->sysoem);
		set_utsfield(it->numusers, "unlim", 0);
		__put_user(1, &it->numcpu);
	}
	up_read(&uts_sem);

	return error;
}


int v7_utsname(unsigned long addr)
{
	int error;
	struct v7_utsname *it = (struct v7_utsname *)addr;

	down_read(&uts_sem);
	error = verify_area(VERIFY_WRITE, it, sizeof (struct v7_utsname));
	if (!error) {
		set_utsfield(it->sysname, system_utsname.nodename, 1);
		set_utsfield(it->nodename, system_utsname.nodename, 1);
		set_utsfield(it->release, system_utsname.release, 0);
		set_utsfield(it->version, system_utsname.version, 0);
		set_utsfield(it->machine, system_utsname.machine, 0);
	}
	up_read(&uts_sem);

	return error;
}

EXPORT_SYMBOL(v7_utsname);

int abi_utsname(unsigned long addr)
{
	int error;
	struct svr4_utsname *it = (struct svr4_utsname *)addr;

	down_read(&uts_sem);
	error = verify_area(VERIFY_WRITE, it, sizeof (struct svr4_utsname));
	if (!error) {
		set_utsfield(it->sysname, system_utsname.sysname, 0);
		set_utsfield(it->nodename, system_utsname.nodename, 0);
		set_utsfield(it->release, system_utsname.release, 0);
		set_utsfield(it->version, system_utsname.version, 0);
		set_utsfield(it->machine, system_utsname.machine, 0);
	}
	up_read(&uts_sem);

	return error;
}

EXPORT_SYMBOL(abi_utsname);
