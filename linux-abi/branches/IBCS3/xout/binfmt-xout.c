/*
 * Copyright (c) 1994  Mike Jagdis (jaggy@purplet.demon.co.uk)
 * Copyright (c) 2001  Christoph Hellwig (hch@caldera.de)
 */

//#ident "%W% %G%"

/*
 * This file is based upon code written by Al Longyear for the COFF file
 * format which is in turn based upon code written by Eric Youngdale for
 * the ELF object file format. Any errors are most likely my own however.
 */
#define __KERNEL_SYSCALLS__
#include "../include/util/i386_std.h"
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/a.out.h>
#include <linux/errno.h>
#include <linux/signal.h>
#include <linux/binfmts.h>
#include <linux/string.h>
#include <linux/fcntl.h>
#include <linux/personality.h>
#include <linux/file.h>
#include <linux/slab.h>
//#include <linux/unistd.h>
#include <linux/syscalls.h>

#include <asm/uaccess.h>
#include <asm/desc.h>

#include "../include/abi_reg.h"
#include "../include/xout.h"
#include "../include/util/trace.h"
extern unsigned long abi_personality(char *);


MODULE_DESCRIPTION("Support for the Microsoft a.out (x.out) binary format");
MODULE_AUTHOR("Mike Jagdis, Christoph Hellwig");
MODULE_LICENSE("GPL");
MODULE_INFO(supported,"yes");
MODULE_INFO(bugreport,"agon04@users.sourceforge.net");

static char *Emulx286 = NULL;
module_param(Emulx286, charp, 0);
MODULE_PARM_DESC(Emulx286,"Xenix 286 Emulator Path");

/*
 * If you compile with XOUT_DEBUG defined you will get additional
 * debugging messages from the x.out module.
 */

//#define XOUT_DEBUG

/*
 * Be verbose if XOUT_DEBUG is defined.
 */
#if defined(XOUT_DEBUG)
#define dprintk(x...)	printk(x)
#else
#define dprintk(x...)
#endif


static int	xout_load_binary(struct linux_binprm *, struct pt_regs *);
static int	xout_load_library(struct file *);

#if _KSL > 23
static struct linux_binfmt xout_format = {
	{NULL, NULL}, THIS_MODULE, xout_load_binary, xout_load_library, NULL, PAGE_SIZE, 0
};
#else
static struct linux_binfmt xout_format = {
	NULL, THIS_MODULE, xout_load_binary, xout_load_library, NULL, PAGE_SIZE
};
#endif


static u32 *
xout_create_tables(char *p, struct linux_binprm *bprm, int ibcs)
{
        int				argc = bprm->argc, envc = bprm->envc;
        u32				*argv, *envp, *sp;
	char				c;

        sp = (u32 *) ((-(u_long)sizeof(u32)) & (u_long) p);
        sp -= envc+1;
        envp = (u32 *)sp;
        sp -= argc+1;
        argv = (u32 *)sp;
        if (!ibcs) {
                sp--;
                put_user((u_long)envp, sp);
                sp--;
                put_user((u_long)argv, sp);
        }
        sp--;
        put_user(argc, sp);
        current->mm->arg_start = (u_long) p;
        while (argc-->0) {
                put_user((u32)(u_long)p, argv++);
		//p += strlen_user(p);
		while(get_user(c,p++) == 0) if (c == 0) break;
        }
        put_user(0,argv);
        current->mm->arg_end = current->mm->env_start = (u_long) p;
        while (envc-->0) {
                put_user((u32)(u_long)p, envp++);
		//p += strlen_user(p);
		while(get_user(c,p++) == 0) if (c == 0) break;
        }
        put_user(0,envp);

        current->mm->env_end = (u_long) p;
        return (sp);
}

static __inline int
isnotaligned(struct xseg *seg)
{
	dprintk(KERN_DEBUG
		"xout: %04x %04x %04x %02x %08lx %08lx %08lx %08lx\n",
		seg->xs_type, seg->xs_attr, seg->xs_seg, seg->xs_align,
		seg->xs_filpos, seg->xs_psize, seg->xs_vsize, seg->xs_rbase);

#if defined(XOUT_DEBUG)
	if ((seg->xs_filpos - seg->xs_rbase) & ~PAGE_MASK) {
		printk(KERN_DEBUG
			"xout: bad alignment - demand paging disabled\n");
	}
#endif
	return ((seg->xs_filpos & ~PAGE_MASK) | (seg->xs_rbase & ~PAGE_MASK));
}

static __inline__ void
clear_memory(u_long addr, u_long size)
{
	while (size-- != 0)
		put_user(0, (char *)addr++);
}
static int
cap_mmap(int oper)
{
#if _KSL > 28
        struct cred *cred = (struct cred *)(current->cred);
#else
        struct task_struct *cred = current;
#endif
	switch (oper) {
	case 1:
		cap_raise(cred->cap_effective,CAP_SYS_RAWIO);
		break;
	case 2:
		cap_lower(cred->cap_effective,CAP_SYS_RAWIO);
		break;
	}
	return cap_raised(cred->cap_effective,CAP_SYS_RAWIO);
}

static int
xout_amen(struct file *fp, struct xseg *sp, int pageable, u_long *addrp,
		struct xexec *xexec, struct pt_regs *regs, int impure)
{
	u_long			bss_size, bss_base, ce;
	int			err = 0;

	ce = cap_mmap(0);
	cap_mmap(1);

	bss_size = sp->xs_vsize - sp->xs_psize;
	bss_base = sp->xs_rbase + sp->xs_psize;

	/*
	 * If it is a text segment update the code boundary
	 * markers. If it is a data segment update the data
	 * boundary markers.
	 */
	if (sp->xs_type == XS_TTEXT || sp->xs_type == XS_TDATA) {
		if ((sp->xs_rbase + sp->xs_psize) > current->mm->end_code)
			current->mm->end_code = (sp->xs_rbase + sp->xs_psize);
	} 

	if ((sp->xs_rbase + sp->xs_vsize) > current->mm->brk) {
		current->mm->start_brk =
			current->mm->brk = PAGE_ALIGN(sp->xs_rbase + sp->xs_vsize);
	}

	if (!pageable) {
		dprintk(KERN_DEBUG "xout: Null map 0x%08lx, length 0x%08lx\n",
				sp->xs_rbase, sp->xs_vsize);
		down_write(&current->mm->mmap_sem);
		err = do_mmap(NULL, sp->xs_rbase, sp->xs_vsize,
				PROT_READ|PROT_WRITE|PROT_EXEC,
				MAP_FIXED|MAP_PRIVATE|MAP_32BIT, 0);
		up_write(&current->mm->mmap_sem);
		goto out;
	}

	dprintk(KERN_DEBUG "xout: mmap to 0x%08lx from 0x%08lx, length 0x%08lx\n",
			sp->xs_rbase, sp->xs_filpos, sp->xs_psize);
	if (sp->xs_attr & XS_APURE) {
		down_write(&current->mm->mmap_sem);
		err = do_mmap(fp, sp->xs_rbase, sp->xs_psize,
			PROT_READ|PROT_EXEC, MAP_FIXED|MAP_SHARED|MAP_32BIT,
			sp->xs_filpos);
		up_write(&current->mm->mmap_sem);
	} else {
		down_write(&current->mm->mmap_sem);
		err = do_mmap(fp, sp->xs_rbase, sp->xs_psize,
			PROT_READ|PROT_WRITE|PROT_EXEC,
			MAP_FIXED|MAP_PRIVATE | MAP_DENYWRITE | MAP_EXECUTABLE | MAP_32BIT,
			sp->xs_filpos);
		up_write(&current->mm->mmap_sem);
	}

	if (err < 0) goto out;

	/*
	 * Map uninitialised data.
	 */
	if (bss_size) {
		if (bss_base & PAGE_MASK) {
			clear_memory(bss_base, PAGE_ALIGN(bss_base)-bss_base);
			bss_size -= (PAGE_ALIGN(bss_base) - bss_base);
			bss_base = PAGE_ALIGN(bss_base);
		}

		dprintk(KERN_DEBUG "xout: Null map 0x%08lx, length 0x%08lx\n",
				bss_base, bss_size);

		down_write(&current->mm->mmap_sem);
		err = do_mmap(NULL, bss_base, bss_size,
				PROT_READ | PROT_WRITE | PROT_EXEC,
				MAP_FIXED | MAP_PRIVATE | MAP_32BIT, 0);
		up_write(&current->mm->mmap_sem);
	}

out:
	if (!ce) cap_mmap(2);
	return (err);
}

/*
 * Helper function to process the load operation.
 */
static int
xout_load_object(struct linux_binprm * bpp, struct pt_regs *rp, int executable)
{
	struct xexec			*xexec = (struct xexec *)bpp->buf;
	struct xext			*xext = (struct xext *)(xexec + 1);
	struct xseg			*seglist;
	struct file			*fp = NULL;
	u_long				addr, lPers;
	int				nsegs, ntext, ndata;
	int				pageable = 1, err = 0, i;
#ifdef CONFIG_BINFMT_XOUT_X286
        struct file			*file;
#endif

	lPers = abi_personality((char *)_BX(rp));
	if (lPers == 0) lPers = PER_XENIX;

	if (xexec->x_magic != X_MAGIC) {
		return -ENOEXEC;
	}

	switch (xexec->x_cpu & XC_CPU) {
		case XC_386:
			break;
#if defined(CONFIG_BINFMT_XOUT_X286)
		case XC_8086:
		case XC_286:
		case XC_286V:
		case XC_186:
		if (!Emulx286) return -ENOEXEC;
		file = open_exec(Emulx286);
		if (file) {
			fput(bpp->file);
			bpp->file = file;
			kernel_read(bpp->file, 0L, bpp->buf, sizeof(bpp->buf));
		}
		return -ENOEXEC;
#endif
		default:
		dprintk(KERN_DEBUG "xout: unsupported CPU type (%02x)\n",
					xexec->x_cpu);
			return -ENOEXEC;
	}

	/*
	 * We can't handle byte or word swapped headers. Well, we
	 * *could* but they should never happen surely?
	 */
	if ((xexec->x_cpu & (XC_BSWAP | XC_WSWAP)) != XC_WSWAP) {
		dprintk(KERN_DEBUG "xout: wrong byte or word sex (%02x)\n",
				xexec->x_cpu);
		return -ENOEXEC;
	}

	/* Check it's an executable. */
	if (!(xexec->x_renv & XE_EXEC)) {
		dprintk(KERN_DEBUG "xout: not executable\n");
		return -ENOEXEC;
	}

	/*
	 * There should be an extended header and there should be
	 * some segments. At this stage we don't handle non-segmented
	 * binaries. I'm not sure you can get them under Xenix anyway.
	 */
	if (xexec->x_ext != sizeof(struct xext)) {
		dprintk(KERN_DEBUG "xout: bad extended header\n");
		return -ENOEXEC;
	}

	if (!(xexec->x_renv & XE_SEG) || !xext->xe_segsize) {
		dprintk(KERN_DEBUG "xout: not segmented\n");
		return -ENOEXEC;
	}

	if (!(seglist = kmalloc(xext->xe_segsize, GFP_KERNEL))) {
		printk(KERN_WARNING "xout: allocating segment list failed\n");
		return -ENOMEM;
	}

	err = kernel_read(bpp->file, xext->xe_segpos,
			(char *)seglist, xext->xe_segsize);
	if (err < 0) {
		dprintk(KERN_DEBUG "xout: problem reading segment table\n");
		goto out;
	}

	if (!bpp->file->f_op->mmap)
		pageable = 0;

	nsegs = xext->xe_segsize / sizeof(struct xseg);

	ntext = ndata = 0;
	for (i = 0; i < nsegs; i++) {
		switch (seglist[i].xs_type) {
			case XS_TTEXT:
				if (isnotaligned(seglist+i))
					pageable = 0;
				ntext++;
				break;
			case XS_TDATA:
				if (isnotaligned(seglist+i))
					pageable = 0;
				ndata++;
				break;
		}
	}

	if (!ndata)
		goto out;

	/*
	 * Generate the proper values for the text fields
	 *
	 * THIS IS THE POINT OF NO RETURN. THE NEW PROCESS WILL TRAP OUT SHOULD
	 * SOMETHING FAIL IN THE LOAD SEQUENCE FROM THIS POINT ONWARD.
	 */

	/*
	 *  Flush the executable from memory. At this point the executable is
	 *  committed to being defined or a segmentation violation will occur.
	 */
	if (executable) {
		dprintk(KERN_DEBUG "xout: flushing executable\n");

		flush_old_exec(bpp);

		if ( (lPers & 0xFF) == (current->personality & 0xFF) ) set_personality(0);
		set_personality(lPers);
#if defined(CONFIG_ABI_TRACE)
	abi_trace(ABI_TRACE_UNIMPL, "Personality %08X assigned\n",
				(unsigned int)current->personality);
#endif
#ifdef CONFIG_64BIT
		set_thread_flag(TIF_IA32);
		clear_thread_flag(TIF_ABI_PENDING);
#endif
		current->mm->mmap        = NULL;

#ifdef set_mm_counter
#if _KSL > 14
		set_mm_counter(current->mm, file_rss, 0);
#else
		set_mm_counter(current->mm, rss, 0);
#endif
#else
		current->mm->rss = 0;
#endif
 
#if _KSL > 10
		if ((err = setup_arg_pages(bpp, STACK_TOP, EXSTACK_DEFAULT)) < 0) 
#else
		if ((err = setup_arg_pages(bpp, EXSTACK_DEFAULT)) < 0) 
#endif
		{
			send_sig(SIGSEGV, current, 1);
			return (err);
		}

		bpp->p = (u_long)xout_create_tables((char *)bpp->p, bpp,
				(xexec->x_cpu & XC_CPU) == XC_386 ? 1 : 0);

		current->mm->start_code  = 0;
		current->mm->end_code    = xexec->x_text;
		current->mm->end_data    = xexec->x_text + xexec->x_data;
		current->mm->start_brk   =
		current->mm->brk         = xexec->x_text + xexec->x_data + xexec->x_bss;

#if _KSL > 28
		install_exec_creds(bpp);
#else
 		compute_creds(bpp);
#endif
		current->flags &= ~PF_FORKNOEXEC;

#if _KSL < 15
#ifdef CONFIG_64BIT
		__asm__ volatile (
		"movl %0,%%fs; movl %0,%%es; movl %0,%%ds"
		: :"r" (0));
		__asm__ volatile (
		"pushf; cli; swapgs; movl %0,%%gs; mfence; swapgs; popf"
		: :"r" (0));
		write_pda(oldrsp,bpp->p);
		_FLG(rp) = 0x200;
#else
		__asm__ volatile (
		"movl %0,%%fs ; movl %0,%%gs"
		: :"r" (0));
		_DS(rp) = _ES(rp) = __USER_DS;
#endif
		_SS(rp) = __USER_DS;
		_SP(rp) = bpp->p;
		_CS(rp) = __USER_CS;
		_IP(rp) = xexec->x_entry;
		set_fs(USER_DS);
#else
		start_thread(rp, xexec->x_entry, bpp->p);
#endif
#ifdef CONFIG_64BIT
	__asm__ volatile("movl %0,%%es; movl %0,%%ds": :"r" (__USER32_DS));
	_SS(rp) = __USER32_DS;
	_CS(rp) = __USER32_CS;
#endif

		dprintk(KERN_DEBUG "xout: entry point = 0x%x:0x%08lx\n",
				xext->xe_eseg, xexec->x_entry);


	}
	/*
	 * Scan the segments and map them into the process space. If this
	 * executable is pageable (unlikely since Xenix aligns to 1k
	 * boundaries and we want it aligned to 4k boundaries) this is
	 * all we need to do. If it isn't pageable we go round again
	 * afterwards and load the data. We have to do this in two steps
	 * because if segments overlap within a 4K page we'll lose the
	 * first instance when we remap the page. Hope that's clear...
	 */
	for (i = 0; err >= 0 && i < nsegs; i++) {
		struct xseg		*sp = seglist+i;

		if (sp->xs_attr & XS_AMEM) {
			err = xout_amen(fp, sp, pageable, &addr,
				xexec, rp, (!ntext && ndata == 1));
		}

	}

	/*
	 * We better fix start_data because sys_brk looks there to
	 * calculate data size.
	 * Kernel 2.2 did look at end_code so this is reasonable.
	 */
	if (current->mm->start_data == current->mm->start_code)
		current->mm->start_data = current->mm->end_code;

	dprintk(KERN_DEBUG "xout: start code 0x%08lx, end code 0x%08lx,"
	    " start data 0x%08lx, end data 0x%08lx, brk 0x%08lx\n",
	    current->mm->start_code, current->mm->end_code,
	    current->mm->start_data, current->mm->end_data,
	    current->mm->brk);

	if (pageable)
		goto trap;
	if (err < 0)
		goto trap;

	for (i = 0; (err >= 0) && (i < nsegs); i++) {
		struct xseg		*sp = seglist + i;
		u_long			psize;

		if (sp->xs_type == XS_TTEXT || sp->xs_type == XS_TDATA) {
			dprintk(KERN_DEBUG "xout: read to 0x%08lx from 0x%08lx,"
					" length 0x%8lx\n", sp->xs_rbase,
					sp->xs_filpos, sp->xs_psize);

			if (sp->xs_psize < 0)
				continue;

			/*
			 * Do we still get the size ? Yes! [joerg]
			 */
			psize = kernel_read(bpp->file, sp->xs_filpos,
				(char *)((long)sp->xs_rbase), sp->xs_psize);

			if (psize != sp->xs_psize) {
				dprintk(KERN_DEBUG "xout: short read 0x%8lx\n",psize);
				err = -1;
				break;
			}
		}
	}

	/*
	 * Generate any needed trap for this process. If an error occured then
	 * generate a segmentation violation. If the process is being debugged
	 * then generate the load trap. (Note: If this is a library load then
	 * do not generate the trap here. Pass the error to the caller who
	 * will do it for the process in the outer lay of this procedure call.)
	 */
trap:
	if (executable) {
		if (err < 0) {
			dprintk(KERN_DEBUG "xout: loader forces seg fault "
					"(err = %d)\n", err);
			send_sig(SIGSEGV, current, 0);
		} 
#ifdef CONFIG_PTRACE
		/* --- Red Hat specific handling --- */
#else
		else if (current->ptrace & PT_PTRACED)
			 send_sig(SIGTRAP, current, 0);
#endif
		err = 0;
	}

out:
	kfree(seglist);

	dprintk(KERN_DEBUG "xout: binfmt_xout: result = %d\n", err);

	/*
	 * If we are using the [2]86 emulation overlay we enter this
	 * rather than the real program and give it the information
	 * it needs to start the ball rolling.
	 */
	/*
	 * Xenix 386 programs expect the initial brk value to be in eax
	 * on start up. Hence if we succeeded we need to pass back
	 * the brk value rather than the status. Ultimately the
	 * ret_from_sys_call assembly will place this in eax before
	 * resuming (starting) the process.
	 */
	return (err < 0 ? err : current->mm->brk);
}


/*
 *  This procedure is called by the main load sequence. It will load
 *  the executable and prepare it for execution. It provides the additional
 *  parameters used by the recursive xout loader and tells the loader that
 *  this is the main executable. How simple it is . . . .
 */
static int
xout_load_binary(struct linux_binprm *bpp, struct pt_regs *rp)
{
	int ret;

	ret = xout_load_object(bpp, rp, 1);
	if (ret >= 0 ) SYS(vserver,rp);

	return ret;
}

/*
 * Load the image for any shared library.  This is called when
 * we need to load a library based upon a file name.
 *
 * XXX: I have never seen a Xenix shared library...  --hch
 */
static int
xout_load_library(struct file *fp)
{
	struct linux_binprm		*bpp;
	struct pt_regs			regs;
	int				err = -ENOMEM;

	if (!(bpp = kmalloc(sizeof(struct linux_binprm), GFP_KERNEL))) {
		printk(KERN_WARNING "xout: kmalloc failed\n");
		goto out;
	}

	memset(bpp, 0, sizeof(struct linux_binprm));
	bpp->file = fp;

	if ((err = kernel_read(fp, 0L, bpp->buf, sizeof(bpp->buf))) < 0)
		printk(KERN_WARNING "xout: unable to read library header\n");
	else
		err = xout_load_object(bpp, &regs, 0);

	kfree(bpp);
out:
	return (err);
}

static int __init
binfmt_xout_init(void)
{
	return register_binfmt(&xout_format);
}

static void __exit
binfmt_xout_exit(void)
{
	unregister_binfmt(&xout_format);
}

module_init(binfmt_xout_init);
module_exit(binfmt_xout_exit);
