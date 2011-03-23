#include <linux/kernel.h>
#include <linux/config.h>
#define __NO_VERSION__
#include <linux/module.h>
#include <asm/uaccess.h>
#include <abi/signal.h>
#include <abi/abi.h>
#include <abi/xnx.h>
#include <abi/abi4.h>

#define SC(name)	(void *)__NR_##name

#ifdef CONFIG_ABI_TRACE
#  define ITR(trace, name, args)	,trace,name,args
#else
#  define ITR(trace, name, args)
#endif

/*
 *  Translate the signal number to the corresponding item for Linux.
 */
static inline int abi_mapsig(int sig)
{
	if ((unsigned int) sig >= NSIGNALS)
		return -1;
	return current->exec_domain->signal_map[sig];
}


asmlinkage int sco_sigaction(int sco_signum, const struct sco_sigaction * action,
	struct sco_sigaction * oldaction)
{
	struct sco_sigaction new_sa, old_sa;
	int error, signo;
	mm_segment_t old_fs;
	struct sigaction nsa, osa;

	signo = abi_mapsig(sco_signum);
	if (signo == -1)
		return -EINVAL;

	if (oldaction) {
		error = verify_area(VERIFY_WRITE, oldaction,
				sizeof(struct sco_sigaction));
		if (error)
			return error;
	}

	if (action) {
		error = copy_from_user(&new_sa, action,
				sizeof(struct sco_sigaction));
		if (error)
			return -EFAULT;
		nsa.sa_restorer = NULL;
		nsa.sa_handler = new_sa.sa_handler;
		nsa.sa_mask = map_sigvec_to_kernel(new_sa.sa_mask,
			current->exec_domain->signal_map);
		nsa.sa_flags = SA_NOMASK;
		if (new_sa.sa_flags & SCO_SA_NOCLDSTOP)
			nsa.sa_flags |= SA_NOCLDSTOP;
	}

	old_fs = get_fs();
	set_fs(get_ds());
	error = SYS(rt_sigaction)(signo,
				action ? &nsa : NULL,
				oldaction ? &osa : NULL,
				sizeof(sigset_t));
	set_fs(old_fs);

	if (!error && oldaction) {
		old_sa.sa_handler = osa.sa_handler;
		old_sa.sa_mask = map_sigvec_from_kernel(osa.sa_mask,
			current->exec_domain->signal_invmap);
		old_sa.sa_flags = 0;
		if(osa.sa_flags & SA_NOCLDSTOP)
			old_sa.sa_flags |= SCO_SA_NOCLDSTOP;
		/* This should never fail... */
		copy_to_user(oldaction, &old_sa, sizeof(struct sco_sigaction));
	}
	return error;
}

static ABI_func XNX_funcs[] = {
   { 0,			Ukn	ITR(1, "syscall",	"")	}, /*  0 */
   { xnx_locking,	3	ITR(0, "locking",	"ddd")	}, /*  1 */
   { xnx_creatsem,	2	ITR(1, "creatsem",	"sd")	}, /*  2 */
   { xnx_opensem,	1	ITR(1, "opensem",	"s")	}, /*  3 */
   { xnx_sigsem,	1	ITR(1, "sigsem",	"d")	}, /*  4 */
   { xnx_waitsem,	1	ITR(1, "waitsem",	"d")	}, /*  5 */
   { xnx_nbwaitsem,	1	ITR(1, "nbwaitsem",	"d")	}, /*  6 */
   { xnx_rdchk,		1	ITR(0, "rdchk",		"d")	}, /*  7 */
   { 0,			Ukn	ITR(1, "stkgro",	"")	}, /*  8 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /*  9 */
   { SC(ftruncate),	-2	ITR(0, "chsize",	"dd")	}, /* 10 */
   { xnx_ftime,		1	ITR(0, "ftime",		"x")	}, /* 11 */
   { xnx_nap,		1	ITR(0, "nap",		"d")	}, /* 12 */
   { xnx_sdget,		4	ITR(1, "sdget",		"sddd") }, /* 13 */
   { xnx_sdfree,	1	ITR(1, "sdfree",	"x")	}, /* 14 */
   { xnx_sdenter,	2	ITR(1, "sdenter",	"xd")	}, /* 15 */
   { xnx_sdleave,	1	ITR(1, "sdleave",	"x")	}, /* 16 */
   { xnx_sdgetv,	1	ITR(1, "sdgetv",	"x")	}, /* 17 */
   { xnx_sdwaitv,	2	ITR(1, "sdwaitv",	"xd")	}, /* 18 */
   { 0,			Ukn	ITR(1, "brkctl",	"")	}, /* 19 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /* 20 */
   { 0,			2	ITR(0, "sco-getcwd?",	"dx")	}, /* 21 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /* 22 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /* 23 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /* 24 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /* 25 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /* 26 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /* 27 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /* 28 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /* 29 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /* 30 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /* 31 */
   { xnx_proctl,	3	ITR(0, "proctl",	"ddx")	}, /* 32 */
   { xnx_execseg,	2	ITR(1, "execseg",	"xd")	}, /* 33 */
   { xnx_unexecseg,	1	ITR(1, "unexecseg",	"x")	}, /* 34 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /* 35 */
#ifdef CONFIG_ABI_TRACE
   { abi_select,	5	ITR(0, "select",	"dxxxx")}, /* 36 */
#else
   { SC(_newselect),	-5	ITR(0, "select",	"dxxxx")}, /* 36 */
#endif
   { xnx_eaccess,	2	ITR(0, "eaccess",	"so")	}, /* 37 */
   { xnx_paccess,	5	ITR(1, "paccess",	"dddds")},/* 38 */
   { sco_sigaction,	3	ITR(0, "sigaction",	"dxx")	}, /* 39 */
   { abi_sigprocmask,	3	ITR(0, "sigprocmask",	"dxx")	}, /* 40 */
   { xnx_sigpending,	1	ITR(1, "sigpending",	"x")	}, /* 41 */
   { abi_sigsuspend,	Spl	ITR(0, "sigsuspend",	"x")	}, /* 42 */
   { SC(getgroups),	-2	ITR(0, "getgroups",	"dx")	}, /* 43 */
   { SC(setgroups),	-2	ITR(0, "setgroups",	"dx")	}, /* 44 */
   { ibcs_sysconf,	1	ITR(0, "sysconf",	"d")	}, /* 45 */
   { xnx_pathconf,	2	ITR(0, "pathconf",	"sd")	}, /* 46 */
   { xnx_fpathconf,	2	ITR(0, "fpathconf",	"dd")	}, /* 47 */
   { SC(rename),	-2	ITR(0, "rename",	"ss")	}, /* 48 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /* 49 */
   { sco_utsname,	1	ITR(0, "sco_utsname",	"x")	}, /* 50 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /* 51 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /* 52 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /* 53 */
   { 0,			Ukn	ITR(1, "?",		"")	}, /* 54 */
   { SC(getitimer),	-2	ITR(0, "getitimer",	"dx")	}, /* 55 */
   { SC(setitimer),	-3	ITR(0, "setitimer",	"dxx")	}  /* 56 */
};


void iBCS_class_XNX(struct pt_regs *regs)
{
	int i;
	
	i = regs->eax >> 8;
	if (i > 56) {
		regs->eax = iABI_errors(-EINVAL);
		regs->eflags |= 1;
		return;
	}

	abi_dispatch(regs, &XNX_funcs[i], 1);
	return;
}

EXPORT_SYMBOL(iBCS_class_XNX);
