/*
 * Copyright (c) 2000,2001 Christoph Hellwig.
 * Copyright (c) 2001 Caldera Deutschland GmbH.
 * All rights resered.
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

//#ident "%W% %G%"

/*
 * Lowlevel handler for lcall7-based syscalls.
 */
#include "../include/util/i386_std.h"
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/init.h>
#include <linux/personality.h>
#include <linux/binfmts.h>
#include <linux/highmem.h>
#include <asm/uaccess.h>
#include <asm/desc.h>
#include <asm/msr.h>

#include "../include/abi_reg.h"
#include "../include/util/errno.h"
#include "../include/util/trace.h"
#include "../include/util/sysent.h"

MODULE_AUTHOR("Christoph Hellwig");
MODULE_DESCRIPTION("Lowlevel handler for lcall7-based syscalls");
MODULE_LICENSE("GPL");
MODULE_INFO(supported,"yes");
MODULE_INFO(bugreport,"agon04@users.sourceforge.net");

static char *ExeFlags = NULL;
module_param(ExeFlags, charp, 0);
MODULE_PARM_DESC(ExeFlags,"Personality Flags List");

#if _KSL > 26
DECLARE_RWSEM(uts_sem);
EXPORT_SYMBOL(uts_sem);
#endif

static void get_args(int args[], struct pt_regs *regs, int of, int n)
{
	int i;

	for (i = 0; i < n; i++)
		get_user(args[i], ((unsigned int *)_SP(regs)) + (i+of));
}

/*
 *	lcall7_syscall    -    indirect syscall for the lcall7 entry point
 *
 *	@regs:		saved user registers
 *
 *	This function implements syscall(2) in kernelspace for the lcall7-
 *	based personalities.
 */
typedef __attribute__((regparm(0))) void (*lc_handler_t)(int, struct pt_regs *);

int lcall7_syscall(struct pt_regs *regs)
{
	lc_handler_t h_lcall;
	__get_user(_AX(regs), ((unsigned long *)_SP(regs))+1);

	++_SP(regs);
	h_lcall = (lc_handler_t)current_thread_info()->exec_domain->handler;
	(h_lcall)(-1,regs);
	--_SP(regs);

	return 0;
}

/**
 *	lcall7_dispatch    -    handle lcall7-based syscall entry
 *
 *	@regs:		saved user registers
 *	@ap:		syscall table entry
 *	@off:		argument offset
 *
 *	This function handles lcall7-based syscalls after the personality-
 *	specific rountine selected the right syscall table entry.
 */

void lcall7_dispatch(struct pt_regs *regs, struct sysent *ap, int off)
{
	short nargs = ap->se_nargs;
	unsigned long iSysFunc = (unsigned long)ap->se_syscall;
	int args[8], error;

	if (!ap->se_syscall) /* XXX kludge XXX */
		nargs = Unimpl;

	if (nargs <= ARRAY_SIZE(args))
		get_args(args, regs, off, nargs);

#if defined(CONFIG_ABI_TRACE)
	if (abi_traced(ABI_TRACE_API)) {
		if (nargs == Spl)
			get_args(args, regs, off, strlen(ap->se_args));
		plist(ap->se_name, ap->se_args, args);
	}
#endif

if (iSysFunc > 0 && iSysFunc < 512) {
 error = sys_call(iSysFunc,args[0],args[1],args[2],args[3],args[4],args[5],args[6]);
 }
 else 
	switch (nargs) {
	case Fast:
		SYSCALL_PREGS(ap->se_syscall, regs);
		goto show_signals;
	case Spl:
		error = SYSCALL_PREGS(ap->se_syscall, regs);
		break;
	case 0:
		error = SYSCALL_VOID(ap->se_syscall);
		break;
	case 1:
		error = SYSCALL_1ARG(ap->se_syscall, args);
		break;
	case 2:
		error = SYSCALL_2ARG(ap->se_syscall, args);
		break;
	case 3:
		error = SYSCALL_3ARG(ap->se_syscall, args);
		break;
	case 4:
		error = SYSCALL_4ARG(ap->se_syscall, args);
		break;
	case 5:
		error = SYSCALL_5ARG(ap->se_syscall, args);
		break;
	case 6:
		error = SYSCALL_6ARG(ap->se_syscall, args);
		break;
	case 7:
		error = SYSCALL_7ARG(ap->se_syscall, args);
		break;
	default:
#if defined(CONFIG_ABI_TRACE)
		abi_trace(ABI_TRACE_UNIMPL,
			"Unsupported ABI function 0x%lx (%s)\n",
			_AX(regs), ap->se_name);
#endif
		error = -ENOSYS;
	}

	if (error > -ENOIOCTLCMD && error < 0) {
		set_error(regs, iABI_errors(-error));

#if defined(CONFIG_ABI_TRACE)
		abi_trace(ABI_TRACE_API,
		    "%s error return %d/%ld\n",
		    ap->se_name, error, _AX(regs));
#endif
	} else {
		clear_error(regs);
		set_result(regs, error);

#if defined(CONFIG_ABI_TRACE)
		abi_trace(ABI_TRACE_API,
		    "%s returns %ld (edx:%ld)\n",
		    ap->se_name, _AX(regs), _DX(regs));
#endif
	}

show_signals:
#if defined(CONFIG_ABI_TRACE)
	if (signal_pending(current) && abi_traced(ABI_TRACE_SIGNAL)) {
		unsigned long signr;

		signr = current->pending.signal.sig[0] &
			~current->blocked.sig[0];

		__asm__("bsf %1,%0\n\t"
				:"=r" (signr)
				:"0" (signr));

		__abi_trace("SIGNAL %lu, queued 0x%08lx\n",
			signr+1, current->pending.signal.sig[0]);
	}
#endif
        return;
}
/*------------------------------------------------------*/
static char *abi_perlist = (char *)0;

unsigned long
abi_personality(char *pPath)
{
	char *pName, *s, buf[128]; unsigned long l; int len;

	if (!abi_perlist) return 0;

	if( strncpy_from_user(buf,pPath,128) < 0 ) return 0;

	pName = buf; s = buf;
	while (s[0]!='\0') { if (s[0]=='/') pName = s+1; s++; }
	len = s - pName + 1;

	s = abi_perlist;
	l = 0;
	while (s[0] != '\0') {
		if ( memcmp(s+1,pName,len) == 0 ) {
			s = s + (int)(s[0]) - sizeof(long);
			l = *( (unsigned long *)s );
			break;
		}
		s = s + (int)(s[0]);
	}
	return l;
}
			
#define ELF_HEAD	0x464C457F
#define ELF_UXW7	0x314B4455
#define ELF_OSR5	0x3552534F
#define ELF_SVR4	0x34525653
#define ELF_X286	0x36383258

static asmlinkage void
linux_lcall7(int segment, struct pt_regs *regs)
{
	char buf[40];
	int iFD;
	mm_segment_t fs;
	u_int *lMarks;
	long lPers, nPers;
	lc_handler_t h_pers;

#if defined(CONFIG_ABI_TRACE)
	if ( (_OAX(regs) & 0xFF00) == 0xFF00 ) {
		_AX(regs) = abi_trace_flg;
		abi_trace_flg = _OAX(regs) & 0x00FF;
		printk("ABI Trace Flag: %02lX\n",(ulong)abi_trace_flg);
		return;
	}
#endif
	lPers = PER_SVR4;
	if (segment == 0x27) lPers = PER_SOLARIS;

	sprintf(buf,"/proc/%d/exe",current->pid);
	fs=get_fs(); set_fs(get_ds());
	iFD=SYS(open,buf,0,0);
	SYS(read,iFD,buf,40);
	SYS(close,iFD);
	set_fs(fs);
	lMarks = (u_int *)buf;
	if (lMarks[0] == ELF_HEAD) {
		if (lMarks[9] == ELF_UXW7) lPers = PER_UW7;
		if (lMarks[9] == ELF_OSR5) lPers = PER_OSR5;
		if (lMarks[9] == ELF_X286) lPers = PER_XENIX;
	}
	fs=get_fs(); set_fs(get_ds());
	nPers = abi_personality(current->comm);
	set_fs(fs);
	if (nPers != 0 && nPers != current->personality) lPers = nPers;
	if ( (lPers & 0xFF) == (current->personality & 0xFF) ) set_personality(0);
	set_personality(lPers);

	if(current_thread_info()->exec_domain->handler == (handler_t)linux_lcall7) {
#if defined(CONFIG_ABI_TRACE)
	abi_trace(ABI_TRACE_UNIMPL,"Unable to find Domain %ld\n",lPers&0xFF);
#endif
		_IP(regs) = 0;	/* SegFault please :-) */
		return;
	}
#if defined(CONFIG_ABI_TRACE)
	abi_trace(ABI_TRACE_UNIMPL,"Personality %08lX assigned\n",lPers);
#endif
	if (lPers & MMAP_PAGE_ZERO) { 		/* Better Late than Never */
		down_write(&current->mm->mmap_sem);
			do_mmap(NULL, 0, PAGE_SIZE, PROT_READ | PROT_EXEC,
				MAP_FIXED | MAP_PRIVATE | MAP_32BIT, 0);
		up_write(&current->mm->mmap_sem);
	}
	h_pers = (lc_handler_t)current_thread_info()->exec_domain->handler;
	(h_pers)(segment,regs);
	return;
}

static void
init_perlist(void)
{
	int		i;
	char		*p, *s;
	unsigned long	l;
	mm_segment_t	fs;

	abi_perlist = (char *)0;
	if (!ExeFlags) return;

	fs=get_fs(); set_fs(get_ds());
	i = SYS(open,ExeFlags,0,0);
	if (i >= 0) {
		abi_perlist = (char *)kmalloc(PAGE_SIZE,GFP_KERNEL);
		if (abi_perlist) {
			abi_perlist[1] = '<';
			SYS(read,i,abi_perlist+1,PAGE_SIZE-1);
		}
		SYS(close,i);
	}
	set_fs(fs);

	if (!abi_perlist) return;
	p = abi_perlist; 
	p[PAGE_SIZE-2] = '\n'; p[PAGE_SIZE-1] = '<';

	while(p[1] != '<') {
		s = p+1;
		while (s[0] != '\n') s++;
		p[0] = s-p;
		p  = s; p[0] = '\0';
		s -= 9; s[0] = '\0';
		l  = 0;
		while (1) {
		  s++; i = s[0];
		  if ( i <= '9' && i>= '0' ) { l = l*16 + i - 48; continue; }
		  if ( i <= 'F' && i>= 'A' ) { l = l*16 + i - 55; continue; }
		  if ( i <= 'f' && i>= 'a' ) { l = l*16 + i - 87; continue; }
		  break;
		}
		*((unsigned long *)(p-sizeof(long))) = l;
	}
	return;
}

static struct exec_domain linux_exec_domain = {
	name:		"Lcall7_detection",
	handler:	(handler_t)linux_lcall7,
	pers_low:	99,
	pers_high:	99,
	module:		THIS_MODULE
};
static struct exec_domain *pLinuxExec;
static handler_t old_lcall7;
unsigned long *abi_ldt = (long *)0;

static int
lcall_load_binary(struct linux_binprm *bpp, struct pt_regs *rp)
{
	unsigned long lPers;

	if (!abi_ldt) {
		lPers = (*((long *)(bpp->buf+36))) & 0xFFFFFFFF;
		if (lPers != ELF_X286) return -ENOEXEC;
		if (_DX(rp) != 0) return -ENOEXEC;
		lPers = ((unsigned long *)rp)[-1];
		if ( sys_call(512,lPers) != 0 ) return -ENOEXEC;
		init_perlist();
		return -ENOEXEC;
	}

#ifdef CONFIG_ABI_ELFMARK
	lPers = (*((long *)(bpp->buf+36))) & 0xFFFFFFFF;
	if (lPers == ELF_SVR4 || lPers == ELF_OSR5 ||
	    lPers == ELF_UXW7 || lPers == ELF_X286 )
#endif
	SYS(vserver,rp);
	return -ENOEXEC;
}

#if _KSL > 23
static struct linux_binfmt lcall_format = {
	{NULL, NULL}, THIS_MODULE, lcall_load_binary, NULL, NULL, PAGE_SIZE, 0
};
#else
static struct linux_binfmt lcall_format = {
	NULL, THIS_MODULE, lcall_load_binary, NULL, NULL, PAGE_SIZE
};
#endif

static int __init
lcall_init(void)
{
	int err;
	err = register_exec_domain(&linux_exec_domain);
	if (err != 0) return err;
	for (pLinuxExec = &linux_exec_domain; pLinuxExec; pLinuxExec=pLinuxExec->next)
		if (pLinuxExec->pers_low==0) break;
	unregister_exec_domain(&linux_exec_domain);
	if (!pLinuxExec) return -1;
	old_lcall7 = pLinuxExec->handler;
	pLinuxExec->handler = (handler_t)linux_lcall7;

#if _KSL > 29
	err = insert_binfmt(&lcall_format);
#else
	err = register_binfmt(&lcall_format);
#endif
	return err;
}

static void __exit
lcall_exit(void)
{
	unregister_binfmt(&lcall_format);
	if (abi_perlist) kfree(abi_perlist);
	pLinuxExec->handler = old_lcall7;
	sys_call(512,0);
}

#ifdef CONFIG_XEN
#include <xen/interface/xen.h>
int xen_ldt_entry(void *ldt, int entry, __u32 entry_a, __u32 entry_b)
{
	__u32 *lp = (__u32 *)((char *)ldt + entry * 8);
	maddr_t mach_lp = arbitrary_virt_to_machine(lp);

	return HYPERVISOR_update_descriptor(
		(u64)mach_lp,
		(u64)entry_a | ((u64)entry_b<<32));
}
#endif

void
lcall_ldt(void)
{
	mm_segment_t fs;
	unsigned long *ldt;
	struct user_desc l_e;

	fs=get_fs();

	memset(&l_e,0,sizeof(l_e));
	l_e.entry_number = 6;	/* make sure we can fit lcall 0x7 and 0x27 */
				/* force LDT allocation */
	set_fs(get_ds());
	SYS(modify_ldt,1,&l_e,sizeof(l_e));
	set_fs(fs);

#ifdef MAX_LDT_PAGES
	ldt = (u_long *)kmap(current->mm->context.ldt_pages[0]);
#else
	ldt = (u_long *)current->mm->context.ldt;
#endif
	if(!ldt) return;

#ifdef CONFIG_XEN
	xen_ldt_entry(ldt,0,abi_ldt[0],abi_ldt[1]);
#ifdef CONFIG_64BIT
	xen_ldt_entry(ldt,4,abi_ldt[4],abi_ldt[5]); /* 16-byte call gate */
#else
	xen_ldt_entry(ldt,4,abi_ldt[8],abi_ldt[9]);
#endif
#else
	ldt[0] = abi_ldt[0]; ldt[1] = abi_ldt[1];
#ifdef CONFIG_64BIT
	ldt[4] = abi_ldt[4]; ldt[5] = abi_ldt[5]; /* 16-byte call gate */
#else
	ldt[8] = abi_ldt[8]; ldt[9] = abi_ldt[9];
#endif
#endif
				/* force LDT reload */
	set_fs(get_ds());
	SYS(modify_ldt,1,&l_e,sizeof(l_e));
	set_fs(fs);
}

module_init(lcall_init);
module_exit(lcall_exit);

EXPORT_SYMBOL(lcall7_syscall);
EXPORT_SYMBOL(lcall7_dispatch);
EXPORT_SYMBOL(sys_call);
EXPORT_SYMBOL(abi_personality);
