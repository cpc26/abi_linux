#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <sys/mman.h>

#include "../include/xout.h"
#include "x286emul.h"
#include "ldt.h"
#include "debug.h"

static char x_hdr[sizeof(struct xexec) + sizeof(struct xext)];
/*-----------------------------------------------------------*/
#define PAGE_SHIFT	12
#define PAGE_SIZE	(1UL << PAGE_SHIFT)
#define PAGE_MASK	(~(PAGE_SIZE-1))
#define PAGE_ALIGN(addr)	(((addr)+PAGE_SIZE-1)&PAGE_MASK)

static __inline int
isnotaligned(struct xseg *seg)
{
	return ((seg->xs_filpos & ~PAGE_MASK) | (seg->xs_rbase & ~PAGE_MASK));
}
/*-----------------------------------------------------------*/
static int
xout_amen(int fd, struct xseg *sp, int pageable, u_long *addrp,
		struct xexec *xexec, int impure)
{
	struct xext		*xext = (struct xext *)(xexec + 1);
	struct user_ldt		ldt_info;
	u_long			mirror_addr = 0;
	u_long			bss_size, bss_base;
	int			err = 0;
	void			*map;

	d_print("  Segment: type=0x%04x,attr=0x%04x\n",sp->xs_type,sp->xs_attr);
//	if (!sp->xs_vsize) return 0;
	memset(&ldt_info,0,sizeof(ldt_info));
seg_again:
	/*
	 * Xenix 386 segments simply map the whole address
	 * space either read-exec only or read-write.
	 */
	ldt_info.entry_number = sp->xs_seg >> 3;
	ldt_info.read_exec_only = 0 /* ((s->xs_attr & XS_APURE) ? 1 : 0) */;
	ldt_info.contents = ((sp->xs_type == XS_TTEXT) ? LDT_CODE : LDT_DATA);
	ldt_info.seg_not_present = 0;
	ldt_info.seg_32bit = ((sp->xs_attr & XS_A32BIT) ? 1 : 0);
	ldt_info.base_addr = *addrp;
	*addrp = PAGE_ALIGN(*addrp + sp->xs_vsize);
	sp->xs_rbase = ldt_info.base_addr;

	bss_size = sp->xs_vsize - sp->xs_psize;
	bss_base = sp->xs_rbase + sp->xs_psize;

	/*
	 * If it is a text segment update the code boundary
	 * markers. If it is a data segment update the data
	 * boundary markers.
	 */
	if (sp->xs_type == XS_TTEXT) {
	} else if (sp->xs_type == XS_TDATA) {
		/*
		 * If it is the first data segment note that
		 * this is the segment we start in. If this
	 	 * isn't a 386 binary add the stack to the
		 * top of this segment.
		 */
		if (init_cs == init_ds) {
			init_ds = init_ss = sp->xs_seg;
			limit_stk = sp->xs_vsize;
			sp->xs_vsize = 0x10000;
			*addrp = PAGE_ALIGN(ldt_info.base_addr + sp->xs_vsize);
		}
	}


	ldt_info.limit = sp->xs_vsize-1;
	ldt_info.limit_in_pages = 0;

	err = modify_ldt(1, &ldt_info, sizeof(ldt_info));

	if (err < 0) {
		fprintf(stderr,"Modify LDT failed\n");
		goto out;
	}

	if (!pageable) {
		map = mmap((char *)sp->xs_rbase, sp->xs_vsize,
				PROT_READ|PROT_WRITE|PROT_EXEC,
				MAP_FIXED|MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
		d_print("    Map1 0x%08lx(0x%08lx) = 0x%08lx\n",(ulong)sp->xs_rbase,(ulong)sp->xs_vsize,(ulong)map);
		goto out;
	}

	if (sp->xs_attr & XS_APURE) {
		map = mmap((char *)sp->xs_rbase, sp->xs_psize,
				PROT_READ|PROT_EXEC, MAP_FIXED|MAP_SHARED,
				fd, sp->xs_filpos);
		d_print("    Map2 0x%08lx(0x%08lx) = 0x%08lx\n",(ulong)sp->xs_rbase,(ulong)sp->xs_psize,(ulong)map);
	} else {
		map = mmap((char *)sp->xs_rbase, sp->xs_psize,
			PROT_READ|PROT_WRITE|PROT_EXEC,
			MAP_FIXED|MAP_PRIVATE | MAP_DENYWRITE | MAP_EXECUTABLE,
			fd,sp->xs_filpos);
		d_print("    Map3 0x%08lx(0x%08lx) = 0x%08lx\n",(ulong)sp->xs_rbase,(ulong)sp->xs_psize,(ulong)map);
	}

	/*
	 * Map uninitialised data.
	 */
	if (bss_size) {
		if (bss_base & PAGE_MASK) {
			memset((char *)bss_base, 0, PAGE_ALIGN(bss_base)-bss_base);
			bss_size -= (PAGE_ALIGN(bss_base) - bss_base);
			bss_base = PAGE_ALIGN(bss_base);
		}

		map = mmap((char *)bss_base, bss_size,
				PROT_READ | PROT_WRITE | PROT_EXEC,
				MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS,-1,0);
		d_print("    Map4 0x%08lx(0x%08lx) = 0x%08lx\n",(ulong)bss_base,(ulong)bss_size,(ulong)map);
	}

out:
	if (err >= 0 && impure && sp->xs_seg >= 0x47) {
		/*
		 * Uh oh, impure binary.
		 * Mirror this data segment to the text segment
		 */
		*addrp = mirror_addr = sp->xs_rbase;
		sp->xs_seg = xext->xe_eseg;
		sp->xs_type = XS_TTEXT;
		goto seg_again;
	}
	return (err);
}
/*-----------------------------------------------------------*/
void
load_xout(char *prg)
{
	char 		buf[256], *path, *p;
	struct xseg	*seglist, *sp;
	u_long		addr, start_addr, psize;
	int		nsegs, ntext, ndata;
	int		pageable = 1, err = 0, i, fd;

	fd = open(prg,O_RDONLY);
	path = getenv("PATH");
	while( fd < 0 ) {
		p = strchr(path,':');
		if (p) p[0] = '\0';
		strcpy(buf,path);
		strcat(buf,"/");
		strcat(buf,prg);
		fd = open(buf,0);
		if (!p) break;
		p[0] = ':';
		path = p + 1;
	}
	if ( fd < 0 ) {
		fprintf(stderr,"Unable to find %s\n",prg);
		exit(1);
	}
	read(fd,x_hdr,sizeof(x_hdr));
	xexec = (struct xexec *)x_hdr;
	xext  = (struct xext  *)(x_hdr + sizeof(struct xexec));

	if ( xexec->x_magic != X_MAGIC ) {
		fprintf(stderr,"Bad MAGIC in %s\n",prg);
		exit(2);
	}

	/*
	 * We can't handle byte or word swapped headers. Well, we
	 * *could* but they should never happen surely?
	 */
	if ((xexec->x_cpu & (XC_BSWAP | XC_WSWAP)) != XC_WSWAP) {
		fprintf(stderr,"Bad word sex\n");
		exit(3);
	}

	/* Check it's an executable. */
	if (!(xexec->x_renv & XE_EXEC)) {
		fprintf(stderr,"Not an executable\n");
		exit(4);
	}

	/*
	 * There should be an extended header and there should be
	 * some segments. At this stage we don't handle non-segmented
	 * binaries. I'm not sure you can get them under Xenix anyway.
	 */
	if (xexec->x_ext != sizeof(struct xext)) {
		fprintf(stderr,"Bad extended header\n");
		exit(5);
	}

	if (!(xexec->x_renv & XE_SEG) || !xext->xe_segsize) {
		fprintf(stderr,"Not segmented\n");
		exit(6);
	}

	if (!(seglist = malloc(xext->xe_segsize))) {
		fprintf(stderr,"Unable to allocate segment table\n");
		exit(7);
	}

	lseek(fd,xext->xe_segpos,0);
	read(fd, (char *)seglist, xext->xe_segsize);

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

	if (!ndata) {
		fprintf(stderr,"No data segments\n");
		exit(7);
	}

	/*
	 * The code selector is advertised in the header.
	 */
	if ((xexec->x_cpu & XC_CPU) != XC_386) {
		init_cs = init_ds = init_ss = xext->xe_eseg;
		init_entry = xexec->x_entry;
	} else {
		init_cs = init_ds = init_ss = xext->xe_eseg;
		init_entry = xexec->x_entry;
	}

	/*
	 * Base address for mapping 16bit segments. This should lie above
	 * the emulator overlay.
	 */
	addr = X286_MAP_ADDR;
	addr += 0x1000;

	/*
	 * Scan the segments and map them into the process space. If this
	 * executable is pageable (unlikely since Xenix aligns to 1k
	 * boundaries and we want it aligned to 4k boundaries) this is
	 * all we need to do. If it isn't pageable we go round again
	 * afterwards and load the data. We have to do this in two steps
	 * because if segments overlap within a 4K page we'll lose the
	 * first instance when we remap the page. Hope that's clear...
	 */
	d_print("x286emul: loading %s\n",prg);

	for (i = 0; err >= 0 && i < nsegs; i++) {
		sp = seglist+i;
		if (sp->xs_attr & XS_AMEM) {
			err = xout_amen(fd, sp, pageable, &addr,
				xexec, (!ntext && ndata == 1));
		}
	}

	if (pageable) {
		fprintf(stderr,"No data segments\n");
		exit(8);
	}
	if (err < 0) {
		fprintf(stderr,"xout_amen error\n");
		exit(9);
	}

	for (i = 0; i < nsegs; i++) {
		sp = seglist + i;
		if (sp->xs_type == XS_TTEXT || sp->xs_type == XS_TDATA) {
			if (sp->xs_psize < 0) continue;
			/*
			 * Do we still get the size ? Yes! [joerg]
			 */
			lseek(fd,sp->xs_filpos,0);
			psize = read(fd, (char *)sp->xs_rbase, sp->xs_psize);

			if (psize != sp->xs_psize) {
				fprintf(stderr,"Short read\n");
				exit(10);
			}
		}
	}

	free(seglist);
	close(fd);
/*
	__asm__ volatile ("\tmovl $0x5000007,%%ebx\n"
			"\tmovl $136,%%eax\n"
			"\tint $128\n"
			:
			: );		 personality(PER_XENIX); */
	return;
}
int
pdump(char *s, int l)
{
  char *p, buf[3072], c; int i, j, k;
  if (l<=0) return 0;
  if (l>1024) l = 1024;
  p=buf;
  for(i=0; i<l; i++) {
    c=s[i];
    j = (c & 0xF0) >> 4;
    k = c & 0x0F;
    if (j>9) p[0] = 55 + j;
     else p[0] = 48 + j;
    if (k>9) p[1] = 55 + k;
     else p[1] = 48 + k;
    p[2]='.';
    p += 3;
   }
  p[0]='\0';
  d_print("%s\n",buf); 
  return i;
}
