#include <linux/unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>


#include "x286emul.h"
#include "ldt.h"
#include "debug.h"

char *desc_type[] = {
	"NULL",
	"286TSS",
	"LDT",
	"286BSY",
	"286CGT",
	"TASKGT",
	"286IGT",
	"286TGT",
	"NULL2",
	"386TSS",
	"NULL3",
	"386BSY",
	"386CGT",
	"NULL4",
	"386IGT",
	"386TGT",
	"MEMRO",
	"MEMROA",
	"MEMRW",
	"MEMRWA",
	"MEMROD",
	"MEMRODA",
	"MEMRWD",
	"MEMRWDA",
	"MEMX",
	"MEMXA",
	"MEMXR",
	"MEMXRA",
	"MEMXC",
	"MEMXAC",
	"MEMXRC"
	"MEMXRAC"
};


struct ldt_desc ldt[MAX_SEGMENTS] = { {0, }, };
int last_desc = 0, base_desc = 0;

void
ldt_init(void)
{
	char buf[MAX_SEGMENTS*LDT_ENTRY_SIZE];
	descriptor_t *d = (descriptor_t *)buf;
	int i, count;

	memset(buf, '\0', sizeof(buf));
	memset(ldt, '\0', sizeof(ldt));
	if ((count = modify_ldt(0, buf, sizeof(buf))) < 0) {
		fprintf(stderr, "x286emul: can't get ldt!\n");
		exit(1);
	}
	count = count / LDT_ENTRY_SIZE;

	d_print("SLOT  BASE/SEL    LIM/OFF     TYPE     DPL  ACCESSBITS\n");

	for (i=0; i < count; i++) {
		ldt[i].base = d[i].sd.lobase1
			| (d[i].sd.lobase2 << 16)
			| (d[i].sd.hibase << 24);
		ldt[i].limit = d[i].sd.lolimit | (d[i].sd.hilimit << 16);
		ldt[i].type = d[i].sd.type;
		ldt[i].dpl = d[i].sd.dpl;
		if(ldt[i].limit && i >= (init_ds >>3))
			ldt[i].rlimit = ((unsigned short)ldt[i].limit) | 0x0fff;	/* 4K block */
		else
			ldt[i].rlimit = 0;

		if ((ldt[i].base > 0) || (ldt[i].limit > 0 )) {
			if (i > last_desc)
				last_desc = i;

			if (d[i].ad.type < 16) {
				d_print("%03d   0x%08lx  0x%08lx  %-7s  %03d %s CNT=%d\n",
					i,
					(unsigned long)d[i].gd.selector,
					(unsigned long)d[i].gd.looffset | (d[i].gd.hioffset << 16),
					desc_type[d[i].gd.type],
					d[i].gd.dpl,
					d[i].gd.p ? " PRESENT" : "",
					d[i].gd.stkcpy);
			} else {
				d_print("%03d   0x%08lx  0x%08lx  %-7s  %03d %s%s%s%s%s%s%s\n",
					i,
					ldt[i].base, ldt[i].limit,
					desc_type[ldt[i].type],
					ldt[i].dpl,
					d[i].sd.type & 1 ? " ACCS'D" : "",
					d[i].sd.type & 8 ? " R&X" : " R&W",
					d[i].sd.p ? " PRESENT" : "",
					d[i].sd.user ? " USER" : "",
					d[i].sd.x ? " X" : "",
					d[i].sd.def32 ? " 32" : "",
					d[i].sd.gran ? " PAGES" : "");
			}
		}
	}
}


int
verify_area(int type, unsigned long seg, unsigned long add, unsigned long length)
/* joerg verify_area(int type, unsigned short seg, unsigned short add, unsigned long length) */
{
/*	d_print("--- verify 0x%04lx:0x%04lx",seg,add); */
	if (seg>>3 > last_desc) {
		xerrno = EINVAL;
		return EINVAL;
	}
/*	d_print(" base=0x%08lx, lim=0x%08lx, type=0x%02lx, dbl=0x%02lx, rlim=0x%04lx\n",
			ldt[seg>>3].base, ldt[seg>>3].limit, 
			ldt[seg>>3].type, ldt[seg>>3].dpl, 
			ldt[seg>>3].rlimit); */

	if((!ldt[seg>>3].base) || (!(ldt[seg>>3].type & 0x10))){	/* test for a valid 286 segment */
		xerrno = EINVAL;
		return EINVAL;
	}

	switch(type) {
		case VERIFY_WRITE:
			if((!(ldt[seg>>3].type & 0x02)) || (ldt[seg>>3].type & 0x08)){
				xerrno = EINVAL;
				return EINVAL;
			}		/* fall through */
		case VERIFY_READ:
			if(ldt[seg>>3].limit < (add + length - 1)){
				if(seg != init_ds){	/* check if area is on stack */
					xerrno = EINVAL;
					return EINVAL;
				}
				if((add <= ldt[seg>>3].rlimit) || ((add + length - 1) > 0xffff)){
					xerrno = EINVAL;
					return EINVAL;
				}
			}
			return 0;
		case VERIFY_EXEC:
			if((!(ldt[seg>>3].type & 0x08)) || (ldt[seg>>3].limit < add)){
				xerrno = EINVAL;
				return EINVAL;
			}
			return 0;
		}

	xerrno = EINVAL;
	return EINVAL;
}
