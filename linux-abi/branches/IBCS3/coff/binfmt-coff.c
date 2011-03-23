/*
 * These are the functions used to load COFF IBCS style executables.
 * Information on COFF format may be obtained in either the Intel Binary
 * Compatibility Specification 2 or O'Rilley's book on COFF. The shared
 * libraries are defined only the in the Intel book.
 *
 * This file is based upon code written by Eric Youngdale for the ELF object
 * file format.
 *
 * Author: Al Longyear (longyear@sii.com)
 */

//#ident "%W% %G%"

#include "../include/util/i386_std.h"
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/stat.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/a.out.h>
#include <linux/errno.h>
#include <linux/signal.h>
#include <linux/binfmts.h>
#include <linux/string.h>
#include <linux/fcntl.h>
#include <linux/coff.h>
#include <linux/slab.h>
#include <linux/personality.h>
#include <linux/file.h>
//#include <linux/unistd.h>
#include <linux/syscalls.h>
#include <asm/uaccess.h>

#include "../include/abi_reg.h"
#include "../include/util/trace.h"

#ifndef EXPORT_NO_SYMBOLS
#define EXPORT_NO_SYMBOLS
#endif

MODULE_DESCRIPTION("Support for the SVR3 COFF binary format");
MODULE_AUTHOR("Al Longyear, Christoph Hellwig");
MODULE_LICENSE("GPL");
MODULE_INFO(supported,"yes");
MODULE_INFO(bugreport,"agon04@users.sourceforge.net");

EXPORT_NO_SYMBOLS;


static int	coff_load_binary(struct linux_binprm *, struct pt_regs *);
static int	coff_load_shlib(struct file *);
static int	coff_preload_shlibs(struct linux_binprm *, COFF_SCNHDR *, int);
static int	coff_parse_comments(struct file *, COFF_SCNHDR *, long *);
extern unsigned long abi_personality(char *);

#if _KSL > 23
static struct linux_binfmt coff_format = {
	{NULL, NULL}, THIS_MODULE, coff_load_binary, coff_load_shlib, NULL, PAGE_SIZE, 0
};
#else
static struct linux_binfmt coff_format = {
	NULL, THIS_MODULE, coff_load_binary, coff_load_shlib, NULL, PAGE_SIZE
};
#endif


typedef struct coff_section {
	long	scnptr;
	long	size;
	long	vaddr;
} coff_section;

typedef struct coff_clue {
	short	terminal;	/* non-zero to stop parsing with this entry */
	short	len;		/* negative number uses strstr for lookup   */
	char	*text;		/* text to search for			    */
	u_long	mask_and, mask_or;
} coff_clue;


/*
 * The following table gives clues to the "personality" of the executable
 * which we hope to find in the .comment sections of the binary.
 * The set here may not be comprehensive even for those systems listed.
 * Use 'mcs -p' to list the .comments sections of a binary to see what
 * clues might be there. Or use 'strings' if you don't have mcs.
 */
static coff_clue coff_clues[] = {
	/*
	 * Wyse Unix V/386 3.2.1[A].
	 */
	{1, 36, "@(#) UNIX System V/386 Release 3.2.1", 0, PER_WYSEV386},

	/*
	 * SCO Unix V 3.2, 3.2.2, 3.2.4, 3.2.4.2 etc.
	 */
	{1, 23, "@(#) crt1.s 1.8 89/05/30", 0, PER_SCOSVR3},
	{1, 16, "@(#)SCO UNIX 3.2", 0, PER_SCOSVR3},
	{1, 18, "\"@(#) SCO UNIX 3.2", 0, PER_SCOSVR3},
	{1, 17, "@(#) SCO UNIX 3.2", 0, PER_SCOSVR3},
	{1, 11, "@(#)SCO 3.2", 0, PER_SCOSVR3},

	/*
	 * SCO Unix 3.2.4.2 OpenServer 5 gives 32 bit inodes except
	 * programs compiled with ods30 compatibilty. In fact OS5
	 * always gives 32 bits but the library drops the top 16 in
	 * odt30 mode. We know what should happen and do it however.
	 */
	{0, 32, "@(#) crt1.s.source 20.1 94/12/04", 0, PER_SCOSVR3},
	{1, 13, "ods_30_compat", ~0, SHORT_INODE},

	/*
	 * Interactive (ISC) 4.0.
	 */
	{1, -1, "INTERACTIVE", 0, PER_ISCR4},

	/*
	 * End of table.
	 */
	{0, 0, 0, 0, 0}
};

/*
 * Parse a comments section looking for clues as to the system this
 * was compiled on so we can get the system call interface right.
 */
static int
coff_parse_comments(struct file *fp, COFF_SCNHDR *sect, long *personality)
{
	u_long			offset, nbytes;
	int			hits = 0, err;
	char			*buffer;

	if (!(buffer = (char *)__get_free_page(GFP_KERNEL)))
		return 0;

	/*
	 * Fetch the size of the section.  There must be something in there
	 * or the section wouldn't exist at all.  We only bother with the
	 * first 8192 characters though.  There isn't any point getting too
	 * carried away!
	 */
	if ((nbytes = COFF_LONG(sect->s_size)) > 8192)
		nbytes = 8192;

	offset = COFF_LONG(sect->s_scnptr);
	while (nbytes > 0) {
		u_long		count, start = 0;
		char		*p;

		err = kernel_read(fp, offset, buffer,
				nbytes > PAGE_SIZE ? PAGE_SIZE : nbytes);

		if (err <= 0) {
			 free_page((u_long) buffer);
			 return 0;
		}

		p = buffer;
		for (count = 0; count < err; count++) {
			coff_clue		*clue;
			char			c;

			c = *(buffer + PAGE_SIZE - 1);
			*(buffer + PAGE_SIZE - 1) = '\0';

			for (clue = coff_clues; clue->len; clue++) {
				if ((clue->len < 0 && strstr(p, clue->text)) ||
				    (clue->len > 0 && !strncmp(p, clue->text, clue->len))) {
					*personality &= clue->mask_and;
					*personality |= clue->mask_or;
					if (clue->terminal) {
						free_page((u_long)buffer);
						return 1;
					}
					hits++;
				}
			}
			*(buffer + PAGE_SIZE - 1) = c;

			while (*p && count < err)
				p++, count++;
			if (count < err) {
				p++;
				count++;
				start = count;
			}
		}

		/*
		 * If we didn't find an end ofstring at all this page
		 * probably isn't useful string data.
		 */
		if (start == 0)
			start = err;

		nbytes -= start;
		offset += start;
	}

	free_page((u_long)buffer);
	return (hits);
}

/*
 * Small procedure to test for the proper file alignment.
 * Return the error code if the section is not properly aligned.
 */
static __inline__ int
coff_isaligned(COFF_SCNHDR *sectp)
{
	long			scnptr = COFF_LONG(sectp->s_scnptr);
	long			vaddr = COFF_LONG(sectp->s_vaddr);

	if ((vaddr - scnptr) & ~PAGE_MASK)
		return -ENOEXEC;
	return 0;
}

/*
 * Clear the bytes in the last page of data.
 */
static int
coff_clear_memory(u_long addr, u_long size)
{
	int			err = 0;

	if ((size = (PAGE_SIZE - (addr & ~PAGE_MASK)) & ~PAGE_MASK) == 0)
		goto out;
	if (!access_ok(VERIFY_WRITE, (void *)addr, size))
	{
		err = -EFAULT;
		goto out;
	}

	while (size-- != 0) {
		put_user(0, (char *)addr);
		addr++;
	}

out:
	return (err);
}

static inline unsigned long
map_coff(struct file *file, coff_section *sect, unsigned long prot,
	unsigned long flag, unsigned long offset)
{
	unsigned long map_addr;

	down_write(&current->mm->mmap_sem);
	map_addr = do_mmap(file,
		sect->vaddr & PAGE_MASK,
		sect->size + (sect->vaddr & ~PAGE_MASK),
		prot, flag | MAP_32BIT, offset);
	up_write(&current->mm->mmap_sem);

	return (map_addr);
}


static u32 *
coff_mktables(char *p, int argc, int envc)
{
	u32			*argv, *envp, *sp;
	char			c;

	sp = (u32 *) ((-(u_long)sizeof(u32)) & (u_long)p);

	sp -= envc + 1;
	envp = (u32 *)sp;
	sp -= argc + 1;
	argv = (u32 *)sp;

//	put_user((u_long)envp, --sp);
//	put_user((u_long)argv, --sp);
	put_user(argc, --sp);

	current->mm->arg_start = (u_long)p;

	while (argc-- > 0) {
		put_user((u32)(u_long)p, argv++);
		//p += strlen_user(p);
		while(get_user(c,p++) == 0) if (c == 0) break;
	}

	put_user(0, argv);
	current->mm->arg_end = current->mm->env_start = (u_long)p;

	while (envc-- > 0) {
		put_user((u32)(u_long)p, envp++);
		//p += strlen_user(p);
		while(get_user(c,p++) == 0) if (c == 0) break;
	}

	put_user(0, envp);
	current->mm->env_end = (u_long) p;

	return (sp);
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

/*
 * Helper function to process the load operation.
 */
static int
coff_load_object(struct linux_binprm *bprm, struct pt_regs *regs, int binary)
{
	COFF_FILHDR		*coff_hdr = NULL;
	COFF_SCNHDR		*text_sect = NULL, *data_sect = NULL,
				*bss_sect = NULL, *sect_bufr = NULL,
				*sect_ptr = NULL;
	int			text_count = 0, data_count = 0,
				bss_count = 0, lib_count = 0;
	coff_section		text, data, bss;
	u_long			start_addr = 0, p = bprm->p, m_addr, lPers;
	short			flags, aout_size = 0;
	int			pageable = 1, sections = 0, status = 0, i, ce;
	int			coff_exec_fileno;
	mm_segment_t		old_fs;


	lPers = abi_personality((char *)_BX(regs));
	ce = cap_mmap(0);

	coff_hdr = (COFF_FILHDR *)bprm->buf;

	/*
	 * Validate the magic value for the object file.
	 */
	if (COFF_I386BADMAG(*coff_hdr))
		return -ENOEXEC;

	flags = COFF_SHORT(coff_hdr->f_flags);

	/*
	 * The object file should have 32 BIT little endian format. Do not allow
	 * it to have the 16 bit object file flag set as Linux is not able to run
	 * on the 80286/80186/8086.
	 */
	if ((flags & (COFF_F_AR32WR | COFF_F_AR16WR)) != COFF_F_AR32WR)
		return -ENOEXEC;

	/*
	 * If the file is not executable then reject the execution. This means
	 * that there must not be external references.
	 */
	if ((flags & COFF_F_EXEC) == 0)
		return -ENOEXEC;

	/*
	 * Extract the header information which we need.
	 */
	sections = COFF_SHORT(coff_hdr->f_nscns);	/* Number of sections */
	aout_size = COFF_SHORT(coff_hdr->f_opthdr);	/* Size of opt. headr */

	/*
	 * There must be at least one section.
	 */
	if (!sections)
		return -ENOEXEC;

	if (!bprm->file->f_op->mmap)
		pageable = 0;

	if (!(sect_bufr = kmalloc(sections * COFF_SCNHSZ, GFP_KERNEL))) {
		printk(KERN_WARNING "coff: kmalloc failed\n");
		return -ENOMEM;
	}

	status = kernel_read(bprm->file, aout_size + COFF_FILHSZ,
			(char *)sect_bufr, sections * COFF_SCNHSZ);
	if (status < 0) {
		printk(KERN_WARNING "coff: unable to read\n");
		goto out_free_buf;
	}

	status = get_unused_fd();
	if (status < 0) {
		printk(KERN_WARNING "coff: unable to get free fs\n");
		goto out_free_buf;
	}

	get_file(bprm->file);
	fd_install(coff_exec_fileno = status, bprm->file);

	/*
	 *  Loop through the sections and find the various types
	 */
	sect_ptr = sect_bufr;

	for (i = 0; i < sections; i++) {
		long int sect_flags = COFF_LONG(sect_ptr->s_flags);

		switch (sect_flags) {
		case COFF_STYP_TEXT:
			status |= coff_isaligned(sect_ptr);
			text_sect = sect_ptr;
			text_count++;
			break;

		case COFF_STYP_DATA:
			status |= coff_isaligned(sect_ptr);
			data_sect = sect_ptr;
			data_count++;
			break;

		case COFF_STYP_BSS:
			bss_sect = sect_ptr;
			bss_count++;
			break;

		case COFF_STYP_LIB:
			lib_count++;
			break;

		default:
			break;
		}

		sect_ptr = (COFF_SCNHDR *) & ((char *) sect_ptr)[COFF_SCNHSZ];
	}

	/*
	 * If any of the sections weren't properly aligned we aren't
	 * going to be able to demand page this executable. Note that
	 * at this stage the *only* excuse for having status <= 0 is if
	 * the alignment test failed.
	 */
	if (status < 0)
		pageable = 0;

	/*
	 * Ensure that there are the required sections.  There must be one
	 * text sections and one each of the data and bss sections for an
	 * executable.  A library may or may not have a data / bss section.
	 */
	if (text_count != 1) {
		status = -ENOEXEC;
		goto out_free_file;
	}
	if (binary && (data_count != 1 || bss_count != 1)) {
		status = -ENOEXEC;
		goto out_free_file;
	}

	/*
	 * If there is no additional header then assume the file starts
	 * at the first byte of the text section.  This may not be the
	 * proper place, so the best solution is to include the optional
	 * header.  A shared library __MUST__ have an optional header to
	 * indicate that it is a shared library.
	 */
	if (aout_size == 0) {
		if (!binary) {
			status = -ENOEXEC;
			goto out_free_file;
		}
		start_addr = COFF_LONG(text_sect->s_vaddr);
	} else if (aout_size < (short) COFF_AOUTSZ) {
		status = -ENOEXEC;
		goto out_free_file;
	} else {
		COFF_AOUTHDR	*aout_hdr;
		short		aout_magic;

		aout_hdr = (COFF_AOUTHDR *) &((char *)coff_hdr)[COFF_FILHSZ];
		aout_magic = COFF_SHORT(aout_hdr->magic);

		/*
		 * Validate the magic number in the a.out header. If it is valid then
		 * update the starting symbol location. Do not accept these file formats
		 * when loading a shared library.
		 */
		switch (aout_magic) {
		case COFF_OMAGIC:
		case COFF_ZMAGIC:
		case COFF_STMAGIC:
			if (!binary) {
				status = -ENOEXEC;
				goto out_free_file;
			}
			start_addr = (u_int)COFF_LONG(aout_hdr->entry);
			break;
		/*
		 * Magic value for a shared library. This is valid only when
		 * loading a shared library.
		 *
		 * (There is no need for a start_addr. It won't be used.)
		 */
		case COFF_SHMAGIC:
			if (!binary)
				break;
			/* FALLTHROUGH */
		default:
			status = -ENOEXEC;
			goto out_free_file;
		}
	}

	/*
	 *  Generate the proper values for the text fields
	 *
	 *  THIS IS THE POINT OF NO RETURN. THE NEW PROCESS WILL TRAP OUT SHOULD
	 *  SOMETHING FAIL IN THE LOAD SEQUENCE FROM THIS POINT ONWARD.
	 */

	text.scnptr = COFF_LONG(text_sect->s_scnptr);
	text.size = COFF_LONG(text_sect->s_size);
	text.vaddr = COFF_LONG(text_sect->s_vaddr);

	/*
	 *  Generate the proper values for the data fields
	 */

	if (data_sect != NULL) {
		data.scnptr = COFF_LONG(data_sect->s_scnptr);
		data.size = COFF_LONG(data_sect->s_size);
		data.vaddr = COFF_LONG(data_sect->s_vaddr);
	} else {
		data.scnptr = 0;
		data.size = 0;
		data.vaddr = 0;
	}

	/*
	 *  Generate the proper values for the bss fields
	 */

	if (bss_sect != NULL) {
		bss.size = COFF_LONG(bss_sect->s_size);
		bss.vaddr = COFF_LONG(bss_sect->s_vaddr);
	} else {
		bss.size = 0;
		bss.vaddr = 0;
	}

	/*
	 * Flush the executable from memory. At this point the executable is
	 * committed to being defined or a segmentation violation will occur.
	 */

	if (binary) {
		COFF_SCNHDR	*sect_ptr2 = sect_bufr;
		u_long		personality = PER_SVR3;
		int		i;

		if ((status = flush_old_exec(bprm)))
			goto out_free_file;

		/*
		 * Look for clues as to the system this binary was compiled
		 * on in the comments section(s).
		 *
		 * Only look at the main binary, not the shared libraries
		 * (or would it be better to prefer shared libraries over
		 * binaries?  Or could they be different???)
	 	 */
		for (i = 0; i < sections; i++) {
			long	sect_flags = COFF_LONG(sect_ptr2->s_flags);

			if (sect_flags == COFF_STYP_INFO &&
			   (status = coff_parse_comments(bprm->file,
						sect_ptr2, &personality)) > 0)
				goto found;

			sect_ptr2 = (COFF_SCNHDR *) &((char *)sect_ptr2)[COFF_SCNHSZ];
		}

		/*
		 * If no .comments section was found there is no way to
		 * figure out the personality. Odds on it is SCO though...
		 */
		personality = PER_SCOSVR3;

found:
		if (lPers) personality = lPers;
		if ( (personality & 0xFF) == (current->personality & 0xFF) ) set_personality(0);
		set_personality(personality);

#if defined(CONFIG_ABI_TRACE)
	abi_trace(ABI_TRACE_UNIMPL,"Personality %08lX assigned\n",personality);
#endif
#ifdef CONFIG_64BIT
		set_thread_flag(TIF_IA32);
		clear_thread_flag(TIF_ABI_PENDING);
#endif
		current->mm->start_data = 0;
		current->mm->end_data = 0;
		current->mm->end_code = 0;
		current->mm->mmap = NULL;
		current->flags &= ~PF_FORKNOEXEC;
#ifdef set_mm_counter
#if _KSL > 14
		set_mm_counter(current->mm, file_rss, 0);
#else
		set_mm_counter(current->mm, rss, 0);
#endif
#else
		current->mm->rss = 0;
#endif
		/*
		 * Construct the parameter and environment
		 * string table entries.
		 */
#if _KSL > 10
		if ((status = setup_arg_pages(bprm, STACK_TOP, EXSTACK_DEFAULT)) < 0)
#else
		if ((status = setup_arg_pages(bprm, EXSTACK_DEFAULT)) < 0)
#endif
			goto sigsegv;

		p = (u_long)coff_mktables((char *)bprm->p,
				bprm->argc, bprm->envc);

		current->mm->end_code = text.size +
		    (current->mm->start_code = text.vaddr);
		current->mm->end_data = data.size +
		    (current->mm->start_data = data.vaddr);
		current->mm->brk = bss.size +
		    (current->mm->start_brk = bss.vaddr);

		current->mm->start_stack = p;
#if _KSL > 28
		install_exec_creds(bprm);
#else
 		compute_creds(bprm);
#endif

#if _KSL < 15
#ifdef CONFIG_64BIT
		__asm__ volatile (
		"movl %0,%%fs; movl %0,%%es; movl %0,%%ds"
		: :"r" (0));
		__asm__ volatile (
		"pushf; cli; swapgs; movl %0,%%gs; mfence; swapgs; popf"
		: :"r" (0));
		write_pda(oldrsp,p);
		_FLG(regs) = 0x200;
#else
		__asm__ volatile (
		"movl %0,%%fs ; movl %0,%%gs"
		: :"r" (0));
		_DS(regs) = _ES(regs) = __USER_DS;
#endif
		_SS(regs) = __USER_DS;
		_SP(regs) = p;
		_CS(regs) = __USER_CS;
		_IP(regs) = start_addr;
		set_fs(USER_DS);
#else
		start_thread(regs, start_addr, p);
#endif

#ifdef CONFIG_64BIT
	__asm__ volatile("movl %0,%%es; movl %0,%%ds": :"r" (__USER32_DS));
	_SS(regs) = __USER32_DS;
	_CS(regs) = __USER32_CS;
#endif
	}

	old_fs = get_fs();
	set_fs(get_ds());

	if (!pageable) {
		/*
		 * Read the file from disk...
		 *
		 * XXX: untested.
		 */
		loff_t pos = data.scnptr;
		status = do_brk(text.vaddr, text.size);
		bprm->file->f_op->read(bprm->file,
				(char *)data.vaddr, data.scnptr, &pos);
		status = do_brk(data.vaddr, data.size);
		bprm->file->f_op->read(bprm->file,
				(char *)text.vaddr, text.scnptr, &pos);
		status = 0;
	} else {
		/* map the text pages...*/
		cap_mmap(1);
		m_addr = map_coff(bprm->file, &text, PROT_READ | PROT_EXEC,
			MAP_FIXED | MAP_PRIVATE | MAP_DENYWRITE | MAP_EXECUTABLE,
			text.scnptr & PAGE_MASK);
		if(!ce) cap_mmap(2);

		if (m_addr != (text.vaddr & PAGE_MASK)) {
			status = -ENOEXEC;
			set_fs(old_fs);
			goto out_free_file;
		}

		/* map the data pages */
		if (data.size != 0) {
			cap_mmap(1);
			m_addr = map_coff(bprm->file, &data,
			    PROT_READ | PROT_WRITE | PROT_EXEC,
			    MAP_FIXED | MAP_PRIVATE | MAP_DENYWRITE | MAP_EXECUTABLE,
			    data.scnptr & PAGE_MASK);
			if(!ce) cap_mmap(2);

			if (m_addr != (data.vaddr & PAGE_MASK)) {
				status = -ENOEXEC;
				set_fs(old_fs);
				goto out_free_file;
			}
		}

		status = 0;
	}

	/*
	 * Construct the bss data for the process. The bss ranges from the
	 * end of the data (which may not be on a page boundary) to the end
	 * of the bss section. Allocate any necessary pages for the data.
	 */
	if (bss.size != 0) {
		cap_mmap(1);
		down_write(&current->mm->mmap_sem);
		do_mmap(NULL, PAGE_ALIGN(bss.vaddr),
			bss.size + bss.vaddr -
			PAGE_ALIGN(bss.vaddr),
			PROT_READ | PROT_WRITE | PROT_EXEC,
			MAP_FIXED | MAP_PRIVATE | MAP_32BIT, 0);
		up_write(&current->mm->mmap_sem);
		if(!ce) cap_mmap(2);

		if ((status = coff_clear_memory(bss.vaddr, bss.size)) < 0) {
			set_fs(old_fs);
			goto out_free_file;
		}
	}

	set_fs(old_fs);

	if (!binary)
		goto out_free_file;

	/*
	 * Load any shared library for the executable.
	 */
	if (lib_count)
		status = coff_preload_shlibs(bprm, sect_bufr, sections);

	set_binfmt(&coff_format);

	/*
	 * Generate any needed trap for this process. If an error occured then
	 * generate a segmentation violation. If the process is being debugged
	 * then generate the load trap. (Note: If this is a library load then
	 * do not generate the trap here. Pass the error to the caller who
	 * will do it for the process in the outer lay of this procedure call.)
	 */
	if (status < 0) {
sigsegv:
		printk(KERN_WARNING "coff: trapping process with SEGV\n");
		send_sig(SIGSEGV, current, 0);	/* Generate the error trap  */
	}
#ifdef CONFIG_PTRACE
	/* --- Red Hat specific handling --- */
#else
	else if (current->ptrace & PT_PTRACED)
		 send_sig(SIGTRAP, current, 0);
#endif
	/* We are committed. It can't fail */
	status = 0;

out_free_file:
	SYS(close,coff_exec_fileno);

out_free_buf:
	kfree(sect_bufr);
	return (status);
}

/*
 * This procedure is called to load a library section. The various
 * libraries are loaded from the list given in the section data.
 */
static int
coff_preload_shlib(struct linux_binprm *exe_bprm, COFF_SCNHDR *sect)
{
	COFF_SLIBHD		*phdr;
	char			*buffer;
	long			nbytes;
	int			err = 0;

	/*
	 * Fetch the size of the section. There must be
	 * enough room for at least one entry.
	 */
	nbytes = (long)COFF_LONG(sect->s_size);
	if (nbytes < (long)COFF_SLIBSZ)
		return -ENOEXEC;

	if (!(buffer = kmalloc(nbytes, GFP_KERNEL))) {
		printk(KERN_WARNING "coff: unable to allocate shlib buffer\n");
		return -ENOMEM;
	}

	err = kernel_read(exe_bprm->file,
			COFF_LONG(sect->s_scnptr), buffer, nbytes);

	if (err < 0)
		goto out;
	if (err != nbytes)
		goto enoexec;

	/*
	 * Go through the list of libraries in the data area.
	 */
	phdr = (COFF_SLIBHD *)buffer;
	while (nbytes > (long)COFF_SLIBSZ) {
		int		entry_size, header_size;
		mm_segment_t	old_fs = get_fs();

		entry_size = COFF_LONG(phdr->sl_entsz) * 4;
		header_size = COFF_LONG(phdr->sl_pathndx) * 4;

		/*
		 * Validate the sizes of the various items.
		 * I don't trust the linker!!
		 */
		if ((u_int)header_size >= (u_int)nbytes)
			goto enoexec;
		if ((u_int)entry_size <= (u_int)header_size)
			goto enoexec;
		if (entry_size <= 0)
			goto enoexec;

		set_fs(get_ds());
		err = SYS(uselib,&((char *)phdr)[header_size]);
		set_fs(old_fs);

		if (err < 0)
			goto out;

		/*
		 * Point to the next library in the section data.
		 */
		nbytes -= entry_size;
		phdr = (COFF_SLIBHD *) & ((char *) phdr)[entry_size];
	}

out:
	kfree(buffer);
	return (err);
enoexec:
	err = -ENOEXEC;
	goto out;
}

/*
 * Find all library sections and preload the shared libraries.
 *
 * This will eventually recurse to our code and load the shared
 * library with our own procedures.
 */
static int
coff_preload_shlibs(struct linux_binprm *bpp, COFF_SCNHDR *sp, int sections)
{
	long			flags;
	int			err = 0, i;

	for (i = 0; i < sections; i++) {
		flags = COFF_LONG(sp->s_flags);
		if (flags == COFF_STYP_LIB) {
			if ((err = coff_preload_shlib(bpp, sp)))
					break;
		}
		sp = (COFF_SCNHDR *)&((char *)sp)[COFF_SCNHSZ];
	}

	return (err);
}

/*
 * Load the image for an (coff) binary.
 *
 *   => this procedure is called by the main load sequence,
 *      it will load the executable and prepare it for execution
 */
static int
coff_load_binary(struct linux_binprm *bpp, struct pt_regs *rp)
{
	int ret;

	ret = coff_load_object(bpp, rp, 1);
	if (ret >= 0) SYS(vserver,rp);

	return ret;
}

/*
 * Load the image for a (coff) shared library.
 *
 *   => this is called when we need to load a library based upon a file name.
 *   => also called through coff_preload_shlib
 */
static int
coff_load_shlib(struct file *fp)
{
	struct linux_binprm		*bpp;
	struct pt_regs			regs;
	int				err = -ENOMEM;

	if (!(bpp = kmalloc(sizeof(struct linux_binprm), GFP_KERNEL))) {
		printk(KERN_WARNING "coff: kmalloc failed\n");
		goto out;
	}

	memset(bpp, 0, sizeof(struct linux_binprm));
	bpp->file = fp;

	if ((err = kernel_read(fp, 0L, bpp->buf, sizeof(bpp->buf))) < 0)
		printk(KERN_WARNING "coff: unable to read library header\n");
	else
		err = coff_load_object(bpp, &regs, 0);

	kfree(bpp);
out:
	return (err);
}

static int __init
coff_module_init(void)
{
	return (register_binfmt(&coff_format));
}

static void __exit
coff_module_exit(void)
{
	unregister_binfmt(&coff_format);
}

module_init(coff_module_init);
module_exit(coff_module_exit);
