//#ident "%W% %G%"

#include "../include/util/i386_std.h"
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/personality.h>
#include <linux/sched.h>
#include <linux/syscalls.h>
#include <asm/uaccess.h>
//#include <asm/unistd.h>

#include "../include/abi_reg.h"
#include "../include/svr4/sysent.h"
#include "../include/cxenix/sysent.h"

#include "../include/util/errno.h"
#include "../include/util/sysent.h"
#include "../include/util/trace.h"

#define _K(a) (void *)__NR_##a


MODULE_DESCRIPTION("Xenix/OpenServer cxenix call support");
MODULE_AUTHOR("Christoph Hellwig, partially taken from iBCS");
MODULE_LICENSE("GPL");
MODULE_INFO(supported,"yes");
MODULE_INFO(bugreport,"agon04@users.sourceforge.net");


static struct sysent cxenix_table[] = {
/*   0 */	{ 0,			Ukn,	"syscall",	""	},
/*   1 */	{ xnx_locking,		3,	"locking",	"ddd"	},
/*   2 */	{ xnx_creatsem,		2,	"creatsem",	"sd"	},
/*   3 */	{ xnx_opensem,		1,	"opensem",	"s"	},
/*   4 */	{ xnx_sigsem,		1,	"sigsem",	"d"	},
/*   5 */	{ xnx_waitsem,		1,	"waitsem",	"d"	},
/*   6 */	{ xnx_nbwaitsem,	1,	"nbwaitsem",	"d"	},
/*   7 */	{ xnx_rdchk,		1,	"rdchk",	"d"	},
/*   8 */	{ 0,			Ukn,	"stkgro",	""	},
/*   9 */	{ 0,			Ukn,	"?",		""	},
/*  10 */	{ _K(ftruncate),	2,	"chsize",	"dd"	},
/*  11 */	{ xnx_ftime,		1,	"ftime",	"x"	},
/*  12 */	{ xnx_nap,		1,	"nap",		"d"	},
/*  13 */	{ xnx_sdget,		4,	"sdget",	"sddd"	},
/*  14 */	{ xnx_sdfree,		1,	"sdfree",	"x"	},
/*  15 */	{ xnx_sdenter,		2,	"sdenter",	"xd"	},
/*  16 */	{ xnx_sdleave,		1,	"sdleave",       "x"	},
/*  17 */	{ xnx_sdgetv,		1,	"sdgetv",	"x"	},
/*  18 */	{ xnx_sdwaitv,		2,	"sdwaitv",	"xd"	},
/*  19 */	{ 0,			Ukn,	"brkctl",	""	},
/*  20 */	{ 0,			Ukn,	"?",		""	},
/*  21 */	{ 0,			2,	"sco-getcwd?",	"dx"	},
/*  22 */	{ 0,			Ukn,	"?",		""	},
/*  23 */	{ 0,			Ukn,	"?",		""	},
/*  24 */	{ 0,			Ukn,	"?",		""	},
/*  25 */	{ 0,			Ukn,	"?",		""	},
/*  26 */	{ 0,			Ukn,	"?",		""	},
/*  27 */	{ 0,			Ukn,	"?",		""	},
/*  28 */	{ 0,			Ukn,	"?",		""	},
/*  29 */	{ 0,			Ukn,	"?",		""	},
/*  30 */	{ 0,			Ukn,	"?",		""	},
/*  31 */	{ 0,			Ukn,	"?",		""	},
/*  32 */	{ xnx_proctl,		3,	"proctl",	"ddx"	},
/*  33 */	{ xnx_execseg,		2,	"execseg",	"xd"	},
/*  34 */	{ xnx_unexecseg,	1,	"unexecseg",	"x"	},
/*  35 */	{ 0,			Ukn,	"?",		""	},
#ifdef CONFIG_65BIT
/*  36 */	{ _K(select),		5,	"select",	"dxxxx"	},
#else
/*  36 */	{ _K(_newselect),	5,	"select",	"dxxxx"	},
#endif
/*  37 */	{ xnx_eaccess,		2,	"eaccess",	"so"	},
/*  38 */	{ xnx_paccess,		5,	"paccess",	"dddds"	},
/*  39 */	{ xnx_sigaction,	3,	"sigaction",	"dxx"	},
/*  40 */	{ abi_sigprocmask,	3,	"sigprocmask",	"dxx"	},
/*  41 */	{ xnx_sigpending,	1,	"sigpending",	"x"	},
/*  42 */	{ abi_sigsuspend,	Spl,	"sigsuspend",	"x"	},
/*  43 */	{ _K(getgroups),	2,	"getgroups",	"dx"	},
/*  44 */	{ _K(setgroups),	2,	"setgroups",	"dx"	},
/*  45 */	{ ibcs_sysconf,		1,	"sysconf",	"d"	},
/*  46 */	{ xnx_pathconf,		2,	"pathconf",	"sd"	},
/*  47 */	{ xnx_fpathconf,	2,	"fpathconf",	"dd"	},
/*  48 */	{ _K(rename),		2,	"rename",	"ss"	},
/*  49 */	{ 0,			Ukn,	"?",		""	},
/*  50 */	{ xnx_utsname,		1,	"utsname",	"x"	},
/*  51 */	{ 0,			Ukn,	"?",		""	},
/*  52 */	{ 0,			Ukn,	"?",		""	},
/*  53 */	{ 0,			Ukn,	"?",		""	},
/*  54 */	{ 0,			Ukn,	"?",		""	},
/*  55 */	{ _K(getitimer),	2,	"getitimer",	"dx"	},
/*  56 */	{ _K(setitimer),	3,	"setitimer",	"dxx"	}
};


void cxenix(struct pt_regs *regs)
{
	int sysno = (_AX(regs) & 0xFFFF) >> 8;

	if (sysno >= ARRAY_SIZE(cxenix_table))
		set_error(regs, iABI_errors(-EINVAL));
	else
		lcall7_dispatch(regs, &cxenix_table[sysno], 1);
}

EXPORT_SYMBOL(cxenix);
