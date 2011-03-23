/*
 *  linux/abi/binfmts/binfmt_xout.c
 *
 *  Copyright (C) 1994  Mike Jagdis (jaggy@purplet.demon.co.uk)
 *
 * $Id$
 * $Source$
 *
 * This file is based upon code written by Al Longyear for the COFF file
 * format which is in turn based upon code written by Eric Youngdale for
 * the ELF object file format. Any errors are most likely my own however.
 */
#include <linux/config.h>

#include <linux/module.h>
#include <linux/version.h>
#include <linux/init.h>
#include <asm/uaccess.h>

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
#include <linux/ptrace.h>
#include <linux/malloc.h>
#include <linux/personality.h>
#include <linux/file.h>
#include <asm/ldt.h>

#include <abi/abi.h>
#include <abi/xout.h>

#ifdef XOUT_DEBUG
#include <ibcs/trace.h>
#endif



static unsigned long *
        create_xout_tables(char *p, struct linux_binprm *bprm, int ibcs);



/* If you compile with XOUT_FORCE_PAGE defined (the default)
 * then if all segments are aligned on 1k bounaries within the
 * file and 4k boundaries within the address space we assume
 * that no overlaps occur within the same VM page and the
 * executable can thus be mmapped if the filesystem allows it.
 * I believe this is the case for all 386 small model binaries,
 * 286 binaries can't be demand paged.
 */
#define XOUT_FORCE_PAGE

/* If you compile with XOUT_SEGMENTS defined the loader will take care
 * to set up the LDT as would "real" Xenix. This shouldn't be necessary
 * for most programs but it is just possible that something out there
 * makes assumptions about its environment and uses segment overrides.
 *
 * The default is not to bother setting up the LDT unless we need support
 * for Xenix 286 binaries.
 */
#undef XOUT_SEGMENTS

/* Xenix 286 requires segment handling. */
#ifdef EMU_X286
#define XOUT_SEGMENTS
#endif


static int load_xout_binary(struct linux_binprm *bprm, struct pt_regs *regs);
static int load_xout_library(int fd);

static int load_object(struct linux_binprm *bprm,
			struct pt_regs *regs,
		 	int lib_ok);



static struct linux_binfmt xout_format = {
	NULL, THIS_MODULE, load_xout_binary, load_xout_library, NULL
};


#if defined(XOUT_DEBUG) && defined(XOUT_SEGMENTS)
/* This is borrowed (with minor variations since we are in kernel mode)
 * from the DPMI code for DOSEMU. I don't claim to understand LDTs :-).
 */
void
print_desc(int which)
{
	unsigned long *lp;
	int count;
	unsigned long base_addr, limit;
	int type, dpl, i;

	if (which) {
		lp = (unsigned long *)((struct desc_struct*)(current->mm->segments));
		count = LDT_ENTRIES;
		printk(KERN_DEBUG "XOUT: LDT\n");
	} else {
		lp = (unsigned long *)0x10c860; /* gdt; DANGER! MAGIC! */
		count = 8;
		printk(KERN_DEBUG "XOUT: GDT\n");
	}

	if (!lp)
		return;

	printk(KERN_DEBUG "XOUT: SLOT  BASE/SEL    LIM/OFF     TYPE  DPL  ACCESSBITS\n");
	for (i=0; i < count; i++, lp++) {
		/* First 32 bits of descriptor */
		base_addr = (*lp >> 16) & 0x0000FFFF;
		limit = *lp & 0x0000FFFF;
		lp++;

		/* First 32 bits of descriptor */
		base_addr |= (*lp & 0xFF000000) | ((*lp << 16) & 0x00FF0000);
		limit |= (*lp & 0x000F0000);
		type = (*lp >> 10) & 7;
		dpl = (*lp >> 13) & 3;
		if ((base_addr > 0) || (limit > 0 )) {
			printk(KERN_DEBUG "XOUT: %03d   0x%08lx  0x%08lx  0x%02x  %03d %s%s%s%s%s%s%s\n",
				i,
				base_addr, limit, type, dpl,
				(*lp & 0x100) ? " ACCS'D" : "",
				(*lp & 0x200) ? " R&W" : " R&X",
				(*lp & 0x8000) ? " PRESENT" : "",
				(*lp & 0x100000) ? " USER" : "",
				(*lp & 0x200000) ? " X" : "",
				(*lp & 0x400000) ? " 32" : "",
				(*lp & 0x800000) ? " PAGES" : "");
		}
	}
}
#endif


static inline int
is_properly_aligned(struct xseg *seg)
{
#ifdef XOUT_DEBUG
	if (ibcs_trace & TRACE_XOUT_LD)
		printk(KERN_DEBUG "XOUT: %04x %04x %04x %02x"
			" %08lx %08lx %08lx %08lx\n",
			seg->xs_type, seg->xs_attr, seg->xs_seg,
			seg->xs_align, seg->xs_filpos, seg->xs_psize,
			seg->xs_vsize, seg->xs_rbase);
#endif
	/* If you compile with XOUT_FORCE_PAGE defined (the default)
	 * then if all segments are aligned on 1k bounaries within the
	 * file and 4k boundaries within the address space we assume
	 * that no overlaps occur within the same VM page and the
	 * executable can thus be mmapped if the filesystem allows it.
	 * I believe this is the case for all 386 small model binaries
	 * - which is all we support anyway.
	 */
#ifdef XOUT_FORCE_PAGE
	/* XXXX The file alignment should be dependent on the block size
	 * of the filesystem shouldn't it?
	 */
#  ifdef XOUT_DEBUG
	if ((ibcs_trace & TRACE_XOUT_LD)
	&& ((seg->xs_filpos & 0x3ff) | (seg->xs_rbase & ~PAGE_MASK)))
		printk(KERN_DEBUG "XOUT: bad page alignment"
			" - demand paging disabled\n");
#  endif
	return ((seg->xs_filpos & 0x3ff) | (seg->xs_rbase & ~PAGE_MASK));
#else
#  ifdef XOUT_DEBUG
	if ((ibcs_trace & TRACE_XOUT_LD)
	&& ((seg->xs_filpos - seg->xs_rbase) & ~PAGE_MASK))
		printk(KERN_DEBUG "XOUT: bad page alignment"
			" - demand paging disabled\n");
#  endif
	return ((seg->xs_filpos - seg->xs_rbase) & ~PAGE_MASK);
#endif
}


static inline void
clear_memory(unsigned long addr, unsigned long size)
{
#ifdef XOUT_DEBUG
	if (ibcs_trace & TRACE_XOUT_LD)
		printk(KERN_DEBUG "XOUT: un-initialized storage 0x%08lx,"
			" length 0x%08lx\n",
			addr, size);
#endif
	while (size-- != 0) {
		put_user(0, (char *)addr);
		addr++;
	}
}


/*
 *  Helper function to process the load operation.
 */

static int
load_object (struct linux_binprm * bprm, struct pt_regs *regs, int lib_ok)
{
	struct xexec *xexec = (struct xexec *)bprm->buf;
	struct xext *xext = (struct xext *)(bprm->buf + sizeof(struct xexec));
	struct xseg *seglist;
	int nsegs, ntext, ndata;
	int not_pageable;
	int i;
	int status;
	int fd = -1;
	struct file *fp = NULL;
	unsigned long addr, mirror_addr;

#ifdef XOUT_DEBUG
	if (ibcs_trace & TRACE_XOUT_LD)
		printk(KERN_DEBUG "XOUT: binfmt_xout entry: %s\n",
			bprm->filename);
#endif

	/* Validate the magic value for the object file. */
	if (xexec->x_magic != X_MAGIC) {
#ifdef XOUT_DEBUG
		if (ibcs_trace & TRACE_XOUT_LD)
			printk(KERN_DEBUG "XOUT: bad magic %04x\n",
				xexec->x_magic);
#endif
		return -ENOEXEC;
	}

	/* Check the CPU type. */
	switch (xexec->x_cpu & XC_CPU) {
		case XC_386:
#ifdef EMU_X286
		case XC_8086:	case XC_286:	case XC_286V:
		case XC_186:
#endif
			break;
		default:
#ifdef XOUT_DEBUG
			if (ibcs_trace & TRACE_XOUT_LD)
				printk(KERN_DEBUG "XOUT: unsupported CPU"
					" type %02x\n",
					xexec->x_cpu);
#endif
			return -ENOEXEC;
	}

	/* We can't handle byte or word swapped headers. Well, we
	 * *could* but they should never happen surely?
	 */
	if ((xexec->x_cpu & (XC_BSWAP | XC_WSWAP)) != XC_WSWAP) {
#ifdef XOUT_DEBUG
		if (ibcs_trace & TRACE_XOUT_LD)
			printk(KERN_DEBUG "XOUT: wrong byte or word sex %02x\n",
				xexec->x_cpu);
#endif
		return -ENOEXEC;
	}

	/* Check it's an executable. */
	if (!(xexec->x_renv & XE_EXEC)) {
#ifdef XOUT_DEBUG
		if (ibcs_trace & TRACE_XOUT_LD)
			printk(KERN_DEBUG "XOUT: not executable\n");
#endif
		return -ENOEXEC;
	}

	/* There should be an extended header and there should be
	 * some segments. At this stage we don't handle non-segmented
	 * binaries. I'm not sure you can get them under Xenix anyway.
	 */
	if (xexec->x_ext != sizeof(struct xext)
	|| !(xexec->x_renv & XE_SEG)
	|| !xext->xe_segsize) {
#ifdef XOUT_DEBUG
		if (ibcs_trace & TRACE_XOUT_LD)
			printk(KERN_DEBUG "XOUT: bad extended header"
				" or not segmented\n");
#endif
		return -ENOEXEC;
	}

	/* Read the segment table. */
	seglist = (struct xseg *)kmalloc(xext->xe_segsize, GFP_KERNEL);
	if (!seglist) {
#ifdef XOUT_DEBUG
		if (ibcs_trace & TRACE_XOUT_LD)
			printk(KERN_DEBUG "XOUT: kmalloc failed when"
				" getting segment list\n");
#endif
		return -ENOEXEC;
	}
	status = read_exec(bprm->dentry, xext->xe_segpos,
			(char *)seglist, xext->xe_segsize, 1);
	if (status < 0) {
#ifdef XOUT_DEBUG
		if (ibcs_trace & TRACE_XOUT_LD)
			printk(KERN_DEBUG "XOUT: problem reading segment table\n");
#endif
		kfree(seglist);
		return status;
	}

	nsegs = xext->xe_segsize / sizeof(struct xseg);

	/* Check we have at least one text segment and
	 * determine whether or not it is pageable.
	 */
	if (!bprm->dentry->d_inode->i_fop
	|| !bprm->dentry->d_inode->i_fop->mmap) {
#ifdef XOUT_DEBUG
		if (ibcs_trace & TRACE_XOUT_LD)
			printk(KERN_DEBUG "XOUT: filesystem does not support"
				" mmap - demand paging disabled\n");
#endif
		not_pageable = 1;
	} else {
		not_pageable = 0;
	}
	ntext = ndata = 0;
	for (i=0; i<nsegs; i++) {
		switch (seglist[i].xs_type) {
			case XS_TTEXT:
				not_pageable |= is_properly_aligned(seglist+i);
				ntext++;
				break;
			case XS_TDATA:
				not_pageable |= is_properly_aligned(seglist+i);
				ndata++;
				break;
		}
	}
	if (!ndata) {
		kfree(seglist);
		return -ENOEXEC;
	}

	/*
	 *  Fetch a file pointer to the executable.
	 */
	fd = open_dentry(bprm->dentry, O_RDONLY);
	if (fd < 0) {
		kfree(seglist);
		return fd;
	} else {
		fp = fcheck(fd);
	}

	/*
	 *  Generate the proper values for the text fields
	 *
	 *  THIS IS THE POINT OF NO RETURN. THE NEW PROCESS WILL TRAP OUT SHOULD
	 *  SOMETHING FAIL IN THE LOAD SEQUENCE FROM THIS POINT ONWARD.
	 */

	/*
	 *  Flush the executable from memory. At this point the executable is
	 *  committed to being defined or a segmentation violation will occur.
	 */
	if (lib_ok) {
#ifdef XOUT_DEBUG
		if (ibcs_trace & TRACE_XOUT_LD)
			printk(KERN_DEBUG "XOUT: flushing executable\n");
#endif
		flush_old_exec(bprm);
/*
 *  Define the initial locations for the various items in the new process
 */
		current->mm->mmap        = NULL;
		current->mm->rss         = 0;
/*
 *  Construct the parameter and environment string table entries.
 */
	status = setup_arg_pages(bprm);
	if (status < 0) {
		send_sig(SIGSEGV, current, 1);
		return status;
	}
	bprm->p = (unsigned long)create_xout_tables((char *)bprm->p, bprm,

#ifdef EMU_X286
					(xexec->x_cpu & XC_CPU) == XC_386
						? 1 : 0);
#else
					1);
#endif

/*
 *  Do the end processing once the stack has been constructed
 */
#ifdef XOUT_SEGMENTS
		/* These will be set up later once we've seen the
		 * segments that make the program up.
		 */
		current->mm->start_code  =
		current->mm->end_code    =
		current->mm->end_data    =
		current->mm->start_brk   =
		current->mm->brk         = 0;
#else
		current->mm->start_code  = 0;
		current->mm->end_code    = xexec->x_text;
		current->mm->end_data    = xexec->x_text + xexec->x_data;
		current->mm->start_brk   =
		current->mm->brk         = xexec->x_text + xexec->x_data + xexec->x_bss;
#endif
		compute_creds(bprm);
		current->flags &= ~PF_FORKNOEXEC;
#ifdef XOUT_SEGMENTS
		/* The code selector is advertised in the header. */
		if ((xexec->x_cpu & XC_CPU) != XC_386) {
			regs->ebx = regs->ecx = xext->xe_eseg;
			regs->eax = xexec->x_entry;
		} else {
			regs->xcs = regs->xds = regs->xes = regs->xss = xext->xe_eseg;
			regs->eip = xexec->x_entry;
		}
#else
		regs->xcs = __USER_CS;
		regs->xds = regs->xes = regs->xss = __USER_DS;
		regs->eip = xexec->x_entry;
#endif
#ifdef XOUT_DEBUG
		if (ibcs_trace & TRACE_XOUT_LD)
			printk(KERN_DEBUG "XOUT: entry point = 0x%x:0x%08lx\n",
				xext->xe_eseg, xexec->x_entry);
#endif
		regs->esp            =
		current->mm->start_stack = bprm->p;

		current->personality = PER_XENIX;

		if (current->exec_domain && current->exec_domain->module)
			__MOD_DEC_USE_COUNT(current->exec_domain->module);
		if (current->binfmt && current->binfmt->module)
			__MOD_DEC_USE_COUNT(current->binfmt->module);
		current->exec_domain = lookup_exec_domain(current->personality);
		current->binfmt = &xout_format;
		if (current->exec_domain && current->exec_domain->module)
			__MOD_INC_USE_COUNT(current->exec_domain->module);
		if (current->binfmt && current->binfmt->module)
			__MOD_INC_USE_COUNT(current->binfmt->module);
	}

	/* Base address for mapping 16bit segments. This should lie above
	 * the emulator overlay.
	 */
	addr=X286_MAP_ADDR;

#ifdef EMU_X286
	/* If this isn't a 386 executable we need to load the overlay
	 * library to emulate a [2]86 environment and save the binary
	 * headers for later reference by the emulator.
	 */
	if ((xexec->x_cpu & XC_CPU) != XC_386) {
		mm_segment_t old_fs = get_fs();

    		set_fs (get_ds ());
    		status = SYS(uselib)("/usr/lib/x286emul");
    		set_fs (old_fs);

		status = do_mmap(NULL,
			addr, sizeof(struct xexec)+sizeof(struct xext),
			PROT_READ|PROT_WRITE|PROT_EXEC,
			MAP_FIXED|MAP_PRIVATE,
  			0);
		if (status >= 0) {
			copy_to_user((char *)addr, xexec, sizeof(struct xexec));
			copy_to_user((char *)addr+sizeof(struct xexec), xext, sizeof(struct xext));
			addr = PAGE_ALIGN(addr+sizeof(struct xexec)+sizeof(struct xext));
		}
	}
#endif

	/* Scan the segments and map them into the process space. If this
	 * executable is pageable (unlikely since Xenix aligns to 1k
	 * boundaries and we want it aligned to 4k boundaries) this is
	 * all we need to do. If it isn't pageable we go round again
	 * afterwards and load the data. We have to do this in two steps
	 * because if segments overlap within a 4K page we'll lose the
	 * first instance when we remap the page. Hope that's clear...
	 *
	 * N.B. If you compile with XOUT_FORCE_PAGE defined (the default)
	 * then if all segments are aligned on 1k bounaries within the
	 * file and 4k boundaries within the address space we assume
	 * that no overlaps occur within the same VM page and the
	 * executable can thus be mmapped if the filesystem allows it.
	 * I believe this is the case for all 386 small model binaries
	 * - which is all we support anyway.
	 */
	mirror_addr = 0;
	for (i=0; status >= 0 && i<nsegs; i++) {
		struct xseg *s = seglist+i;

		if (s->xs_attr & XS_AMEM) {
			unsigned long bss_size;
			unsigned long bss_base;
#ifdef XOUT_SEGMENTS
			struct modify_ldt_ldt_s ldt_info;
			struct desc_struct def_ldt;
			int l;
			mm_segment_t old_fs = get_fs();

seg_again:
			l = 0;

			/* Xenix 386 segments simply map the whole address
			 * space either read-exec only or read-write.
			 */
			ldt_info.entry_number = s->xs_seg >> 3;
#if 0
			ldt_info.read_exec_only = ((s->xs_attr & XS_APURE) ? 1 : 0);
#else
			ldt_info.read_exec_only = 0;
#endif
			ldt_info.contents = ((s->xs_type == XS_TTEXT) ? 2 : 0);
			ldt_info.seg_not_present = 0;
			ldt_info.seg_32bit = ((s->xs_attr & XS_A32BIT) ? 1 : 0);
			if (ldt_info.seg_32bit) {
				ldt_info.base_addr = 0;
			} else {
				ldt_info.base_addr = addr;
				addr = PAGE_ALIGN(addr + s->xs_vsize);
				s->xs_rbase = ldt_info.base_addr;
			}
#endif
			bss_size = seglist[i].xs_vsize-seglist[i].xs_psize;
			bss_base = seglist[i].xs_rbase+seglist[i].xs_psize;

			/* If it is a text segment update the code boundary
			 * markers. If it is a data segment update the data
			 * boundary markers.
			 */
			if (s->xs_type == XS_TTEXT) {
#if 0 /* impossible - start_code is 0 */
				if (s->xs_rbase < current->mm->start_code)
					current->mm->start_code = s->xs_rbase;
#endif
				if (s->xs_rbase+s->xs_psize > current->mm->end_code)
					current->mm->end_code = s->xs_rbase+s->xs_psize;
			} else if (s->xs_type == XS_TDATA) {
#ifdef XOUT_SEGMENTS
				/* If it is the first data segment note that
				 * this is the segment we start in. If this
	 			 * isn't a 386 binary add the stack to the
				 * top of this segment.
				 */
				if ((xexec->x_cpu & XC_CPU) != XC_386) {
					if (regs->ebx == regs->ecx) {
						regs->ecx = s->xs_seg;
						regs->edx = s->xs_vsize;
						s->xs_vsize = 0x10000;
						addr = PAGE_ALIGN(ldt_info.base_addr + s->xs_vsize);
					}
				} else {
					if (regs->xds == regs->xcs)
						regs->xds = regs->xes
						= regs->xss = s->xs_seg;
				}
#endif
				if (s->xs_rbase+s->xs_psize > current->mm->end_data)
					current->mm->end_data = s->xs_rbase+s->xs_psize;
			}
			if (s->xs_rbase+s->xs_vsize > current->mm->brk)
				current->mm->start_brk =
				current->mm->brk = PAGE_ALIGN(s->xs_rbase+s->xs_vsize);
#ifdef XOUT_SEGMENTS
			if (ldt_info.seg_32bit) {
				ldt_info.limit = (TASK_SIZE-1) >> 12;
				ldt_info.limit_in_pages = 1;
			} else {
				ldt_info.limit = s->xs_vsize-1;
				ldt_info.limit_in_pages = 0;
			}
#ifdef XOUT_DEBUG
			if (ibcs_trace & TRACE_XOUT_LD)
				printk(KERN_DEBUG "XOUT: ldt %02x, type=%d,"
					" base=0x%08lx, limit=0x%08x,"
					" pages=%d, 32bit=%d\n",
					ldt_info.entry_number, ldt_info.contents,
					ldt_info.base_addr, ldt_info.limit,
					ldt_info.limit_in_pages, ldt_info.seg_32bit);
#endif
			/* Use the modify_ldt syscall since this allocates
			 * the initial space for the LDT table, tweaks the
			 * GDT etc. We need to read the current LDT first
			 * since we need to copy the lcall7 call gate.
			 */
			set_fs(get_ds());
			if (!current->mm->segments) {
				SYS(modify_ldt)(0, &def_ldt, sizeof(def_ldt));
				l = 1;
			}
			status = SYS(modify_ldt)(1, &ldt_info, sizeof(ldt_info));
#if 0
			if (status >= 0 && !ntext && s->xs_seg == 0x47) {
				/* Uh oh, impure binary... */
				ldt_info.entry_number = 0x3f >> 3;
#if 0
				ldt_info.read_exec_only = 1;
#else
				ldt_info.read_exec_only = 0;
#endif
				ldt_info.contents = 2;
				status = SYS(modify_ldt)(1, &ldt_info, sizeof(ldt_info));
			}
#endif
			set_fs(old_fs);
			if (l == 1) {
				l = 0;
				((struct desc_struct*)(current->mm->segments))[0].a = def_ldt.a;
				((struct desc_struct*)(current->mm->segments))[0].b = def_ldt.b;
			}
			if (status < 0) {
				printk(KERN_INFO "XOUT: modify_ldt returned %d\n", status);
			}
#endif

			if (status >= 0) {
				if (not_pageable) {
#ifdef XOUT_DEBUG
					if (ibcs_trace & TRACE_XOUT_LD)
						printk(KERN_DEBUG "XOUT: Null map 0x%08lx, length 0x%08lx\n",
							s->xs_rbase, s->xs_vsize);
#endif
					status = do_mmap(NULL,
  						s->xs_rbase,
						s->xs_vsize,
						PROT_READ|PROT_WRITE|PROT_EXEC,
						MAP_FIXED|MAP_PRIVATE,
  						0);
				} else {
#ifdef XOUT_DEBUG
					if (ibcs_trace & TRACE_XOUT_LD)
						printk(KERN_DEBUG "XOUT: mmap to 0x%08lx from 0x%08lx, length 0x%08lx\n",
							s->xs_rbase, s->xs_filpos, s->xs_psize);
#endif
					status = do_mmap(fp,
  						s->xs_rbase, s->xs_psize,
						(s->xs_attr & XS_APURE)
  							? PROT_READ|PROT_EXEC
							: PROT_READ|PROT_WRITE|PROT_EXEC,
						(s->xs_attr & XS_APURE)
  							? MAP_FIXED|MAP_SHARED
							: MAP_FIXED|MAP_PRIVATE | MAP_DENYWRITE | MAP_EXECUTABLE,
  						s->xs_filpos);

					/* Map uninitialised data. */
					if (status >= 0 && bss_size && (bss_base & PAGE_MASK)) {
						clear_memory(bss_base,
							PAGE_ALIGN(bss_base)-bss_base);
						bss_size -= (PAGE_ALIGN(bss_base)-bss_base);
						bss_base = PAGE_ALIGN(bss_base);
					}
					if (status >= 0 && bss_size) {
#ifdef XOUT_DEBUG
						if (ibcs_trace & TRACE_XOUT_LD)
							printk(KERN_DEBUG "XOUT: Null map 0x%08lx, length 0x%08lx\n",
								bss_base, bss_size);
#endif
						status = do_mmap(NULL,
								bss_base, bss_size,
								PROT_READ | PROT_WRITE | PROT_EXEC,
								MAP_FIXED | MAP_PRIVATE, 0);
					}
				}
			}
#ifdef XOUT_SEGMENTS
			if (status >= 0 && !ntext && ndata == 1 && s->xs_seg >= 0x47) {
				/* Uh oh, impure binary. Mirror this data
				 * segment to the text segment.
				 */
 				addr = mirror_addr = s->xs_rbase;
 				s->xs_seg = xext->xe_eseg;
				s->xs_type = XS_TTEXT;
				goto seg_again;
			}
#endif
		}
	}

#if 0
	if (addr > current->mm->brk)
		current->mm->start_brk = current->mm->brk = addr;
#endif

#ifdef XOUT_DEBUG
	if (ibcs_trace & TRACE_XOUT_LD) {
		printk(KERN_DEBUG "XOUT: start code 0x%08lx, end code 0x%08lx,"
			" end data 0x%08lx, brk 0x%08lx\n",
			current->mm->start_code, current->mm->end_code,
			current->mm->end_data, current->mm->brk);
#ifdef XOUT_SEGMENTS
		print_desc(1);
		print_desc(0);
#endif
	}
#endif

	if (status >= 0 && not_pageable) {
		for (i=0; status >= 0 && i<nsegs; i++) {
			if (seglist[i].xs_type == XS_TTEXT
			|| seglist[i].xs_type == XS_TDATA) {
#ifdef XOUT_DEBUG
				if (ibcs_trace & TRACE_XOUT_LD)
					printk(KERN_DEBUG "XOUT: read to 0x%08lx from 0x%08lx, length 0x%08lx\n",
						seglist[i].xs_rbase, seglist[i].xs_filpos, seglist[i].xs_psize);
#endif
				if (seglist[i].xs_psize > 0
				&& (unsigned long)read_exec(bprm->dentry, seglist[i].xs_filpos,
					(char *)seglist[i].xs_rbase,
					seglist[i].xs_psize, 0) != seglist[i].xs_psize) {
#ifdef XOUT_DEBUG
					if (ibcs_trace & TRACE_XOUT_LD)
						printk(KERN_DEBUG "XOUT: short read\n");
#endif
					status = -1;
				}
#if 0
				else if (mirror_addr
				&& seglist[i].xs_seg == 0x3f
				&& (unsigned long)read_exec(bprm->dentry, seglist[i].xs_filpos,
				(char *)mirror_addr,
				seglist[i].xs_psize, 0) != seglist[i].xs_psize) {
#ifdef XOUT_DEBUG
					if (ibcs_trace & TRACE_XOUT_LD)
						printk(KERN_DEBUG "XOUT: short read\n");
#endif
					status = -1;
				}
#endif
			}
		}
	}

/*
 *   Generate any needed trap for this process. If an error occured then
 *   generate a segmentation violation. If the process is being debugged
 *   then generate the load trap. (Note: If this is a library load then
 *   do not generate the trap here. Pass the error to the caller who
 *   will do it for the process in the outer lay of this procedure call.)
 */
	if (lib_ok) {
		if (status < 0) {
#ifdef XOUT_DEBUG
			if (ibcs_trace & TRACE_XOUT_LD)
				printk(KERN_DEBUG "XOUT: loader forces seg fault (status=%d)\n",
					status);
#endif
			send_sig(SIGSEGV, current, 0);
		} else if (current->flags & PF_PTRACED) {
			send_sig(SIGTRAP, current, 0);
		}
		status = 0;
	}

	SYS(close)(fd);

	kfree(seglist);

#ifdef XOUT_DEBUG
	if (ibcs_trace & TRACE_XOUT_LD)
		printk(KERN_DEBUG "XOUT: binfmt_xout: result = %d\n",
			status);
#endif

	/* If we are using the [2]86 emulation overlay we enter this
	 * rather than the real program and give it the information
	 * it needs to start the ball rolling.
	 */
	if ((xexec->x_cpu & XC_CPU) != XC_386) {
#if 0
		regs->eax = regs->eip;
		regs->ebx = regs->xcs;
		regs->ecx = regs->xds;
		regs->xcs = __USER_CS;
		regs->xds = regs->xes = regs->xss = __USER_DS;
#endif
		regs->eip = 0x1020;
#ifdef XOUT_DEBUG
		if (ibcs_trace & TRACE_XOUT_LD)
			printk(KERN_DEBUG "XOUT: x286emul 0x%02lx:0x%04lx,"
				" ds=0x%02lx, stack 0x%02lx:0x%04lx\n",
				regs->ebx, regs->eax, regs->ecx, regs->ecx,
				regs->edx);
#endif
#ifdef XOUT_DEBUG
		if (ibcs_trace & TRACE_XOUT_DB)
			while (!signal_pending(current)) schedule();
#endif
		return (status < 0 ? status : regs->eax);
	}

#ifdef XOUT_DEBUG
	if (ibcs_trace & TRACE_XOUT_DB)
		while (!signal_pending(current)) schedule();
#endif
	/* Xenix 386 programs expect the initial brk value to be in eax
	 * on start up. Hence if we succeeded we need to pass back
	 * the brk value rather than the status. Ultimately the
	 * ret_from_sys_call assembly will place this in eax before
	 * resuming (starting) the process.
	 */
	return (status < 0 ? status : current->mm->brk);
}


/*
 *  This procedure is called by the main load sequence. It will load
 *  the executable and prepare it for execution. It provides the additional
 *  parameters used by the recursive xout loader and tells the loader that
 *  this is the main executable. How simple it is . . . .
 */

static int
load_xout_binary(struct linux_binprm *bprm, struct pt_regs *regs)
{
	int ret;

	MOD_INC_USE_COUNT;
	ret = load_object(bprm, regs, 1);
	MOD_DEC_USE_COUNT;
	return ret;
}

/*
 *   Load the image for any shared library.
 *
 *   This is called when we need to load a library based upon a file name.
 */

static int
load_xout_library(int fd)
{
	struct linux_binprm *bprm;  /* Parameters for the load operation   */
	int    status;              /* Status of the request               */

	MOD_INC_USE_COUNT;

	bprm = (struct linux_binprm *)kmalloc (sizeof (struct linux_binprm),
					    GFP_KERNEL);
	if (!bprm) {
#ifdef XOUT_DEBUG
		if (ibcs_trace & TRACE_XOUT_LD)
			printk(KERN_DEBUG "XOUT: kmalloc failed\n");
#endif
		status = -ENOEXEC;
	} else {
		struct file *file;
		struct pt_regs regs;

		memset(bprm, '\0', sizeof (struct linux_binprm));

		file           = fget(fd);
		bprm->dentry   = file->f_dentry;
		bprm->filename = "";

		status = read_exec(bprm->dentry, 0L,
			    bprm->buf, sizeof (bprm->buf), 1);

		status = load_object(bprm, &regs, 0);

		fput(file);
		kfree(bprm);
	}

	MOD_DEC_USE_COUNT;

	return (status);
}


static unsigned long *
create_xout_tables(char *p, struct linux_binprm *bprm, int ibcs)
{
        unsigned long *argv,*envp;
        unsigned long * sp;
        int argc = bprm->argc;
        int envc = bprm->envc;

        sp = (unsigned long *) ((-(unsigned long)sizeof(char *)) & (unsigned long) p);
        sp -= envc+1;
        envp = sp;
        sp -= argc+1;
        argv = sp;
        if (!ibcs) {
                sp--;
                put_user(envp, sp);
                sp--;
                put_user(argv, sp);
        }
        sp--;
        put_user(argc, sp);
        current->mm->arg_start = (unsigned long) p;
        while (argc-->0) {
                put_user(p, argv); argv++;
                p += strlen_user(p);
        }
        put_user(NULL,argv);
        current->mm->arg_end = current->mm->env_start = (unsigned long) p;
        while (envc-->0) {
                put_user(p, envp); envp++;
                p += strlen_user(p);
        }
        put_user(NULL,envp);
        current->mm->env_end = (unsigned long) p;
        return sp;
}




#ifdef MODULE
#define binfmt_xout_init init_module

void cleanup_module(void)
{
  unregister_binfmt(&xout_format);
}
#endif


int __init binfmt_xout_init(void)
{
  register_binfmt(&xout_format);
  return 0;
}

#ifndef MODULE
__initcall(binfmt_xout_init);
#endif
