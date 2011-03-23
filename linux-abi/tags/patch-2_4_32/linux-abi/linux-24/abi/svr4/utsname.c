/*
 * Copyright (C) 1994 Mike Jagdis (jaggy@purplet.demon.co.uk)
 * Copyright (C) 1994 Eric Youngdale.
 */

#ident "%W% %G%"

#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/utsname.h>
#include <asm/uaccess.h>


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
