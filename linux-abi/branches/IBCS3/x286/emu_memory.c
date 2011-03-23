#include <errno.h>
#include <signal.h>
#include <linux/unistd.h>
#include <stdlib.h>
#include <unistd.h>

#include "../include/xout.h"
#include "x286emul.h"
#include "syscall.h"
#include "ldt.h"
#include "debug.h"

int int_brkctl(struct sigcontext *, int , int , int );

/*
 * System call stkgro()
 * called to check if the stack has overflowed
 * the program should "kill()" itselph if we
 * return an error
 *
 * You only need a fixed size stack if you
 * put the stack just above the DATA, like 386
 * Xenix does for 286 programs.
 *
 * Most programs programs just add 0x100 to
 * the "sp" and then pass that as the size of stack 
 * that they require.
 */
void pldt(void);

int
emu_stkgro(struct sigcontext *sc)
{
	unsigned short *stkladdr, required;

	d_print("ebx: 0x%08lx ds: 0x%04x LDATA: %d\n", sc->ebx, sc->ds, LDATA);
	pldt();
	d_print("------------------------------\n");

	if(LDATA){	/* Large Data */
		stkladdr = (unsigned short *)(ldt[sc->ss >> 3].base + (sc->ebx & 0xffff));
		required = stkladdr[0];
	}
	else {
		required = sc->ebx & 0xffff;
	}

	if ((required <= ldt[base_desc].limit) || (required >= init_stk)) {
		xerrno = ENOMEM;
		return -1;	/* cant have a stack that size */
	}

	if(required <= (init_stk - xext->xe_stksize))
		ldt[base_desc].rlimit = required - 1;	/* ok allow for that size stack */

	return 0;
}


#define BR_ARGSEG	1
#define BR_NEWSEG	2
#define BR_IMPSEG	3
#define BR_FREESEG	4
#define BR_HUGE		64
int
emu_brk(struct sigcontext *sc)
{
	int seg, ofset, result;
	unsigned short *stkladdr;
	struct sigcontext st;

	if(LDATA){	/* Large Data */
		stkladdr = (unsigned short *)(ldt[sc->ss >> 3].base + (sc->ebx & 0xffff));
		ofset = stkladdr[0];
		seg = (stkladdr[1] >> 3);
	}
	else {
		ofset = (sc->ebx & 0xffff);
		seg = ((sc->ds & 0xffff) >> 3);
	}
	if (seg >= MAX_SEGMENTS) { xerrno = EINVAL; return -1; }
	ofset = ofset - ldt[seg].limit -1;
	result = int_brkctl(&st, BR_ARGSEG, ofset, seg);
	if(result == -1)
		return result;
	return 0;
}

/*
 * NOTE!
 * "limit" is the maximum ofset in a segment
 * ie it is one less than the segment size!
 * a segment limit of 0 is 1 long....
 * "rlimit" is the max "limit" in that
 * segment.
 */

int
emu_brkctl(struct sigcontext *sc)
{
	int cmd, incr, seg;

	unsigned short *stkladdr;

	if(LDATA){	/* Large Data */
		stkladdr = (unsigned short *)(ldt[sc->ss >> 3].base + (sc->ebx & 0xffff));
		cmd = stkladdr[0];
		incr = (int)(stkladdr[1] + ((int)stkladdr[2] << 16));
		seg = (stkladdr[4] >> 3);
	}
	else {
		cmd = (sc->ebx & 0xffff);
		incr = (int)((sc->ecx & 0xffff) + (sc->esi << 16));
		seg = ((sc->edi & 0xffff) >> 3);
	}
	return int_brkctl(sc, cmd, incr, seg);
}


int
int_brkctl(struct sigcontext *sc, int cmd, int incr, int seg)
{
	struct user_ldt ldt_info;
	int t_last;
	int t_cand;
	int my_last;

	t_last = last_desc;
	my_last = last_desc;
	t_cand = 0;

	sc->ebx =0xff;
	xerrno = EINVAL;

	d_print("x286emulm:   CMD: %d incr: %d seg: %d ldt[seg].limit: %ld\n", cmd, incr, seg, ldt[seg].limit);
	pldt(); 

	if (cmd == BR_ARGSEG && (seg < base_desc || seg > last_desc))
		return -1;

	if (incr > 0x00010000
	|| (cmd == BR_NEWSEG && incr < 0)
/* joerg allow to free all mem 	|| (cmd == BR_ARGSEG && incr < 0 && -incr > ldt[seg].limit)) */
	|| (cmd == BR_ARGSEG && incr < 0 && -incr > ldt[seg].limit + 1))
		return -1;

	if (cmd == BR_IMPSEG)
		seg = last_desc;

	if (incr >= 0) {
		if (cmd == BR_ARGSEG && (unsigned long)(ldt[seg].limit + incr) > ldt[seg].rlimit) {
			return -1;
		}

		if (cmd == BR_NEWSEG || (cmd == BR_IMPSEG && (ldt[seg].limit + incr) > ldt[seg].rlimit)) {
			if (!LDATA && last_desc == base_desc && cmd == BR_IMPSEG)
				return -1;	
			seg = ++last_desc;
			/* never more segments (joerg) */
			if (seg > MAX_SEGMENTS -1) {
				xerrno = ENOMEM;
				last_desc--;
				return -1;
			}
            if((ldt[seg].base = (unsigned long) malloc(0x10000)) == (unsigned long) NULL) {
				last_desc--;
				return -1;
			}

			ldt[seg].limit = -1;
			ldt[seg].rlimit = 0xffff;
			ldt[seg].type = 0x12;
			ldt[seg].dpl = 0x03;

			sc->ebx = (seg << 3) | 7;
			sc->eax = 0;
		} else {
			if(ldt[seg].limit == 0xffff){
				sc->ebx = ((seg + 1) << 3) | 7;
				sc->eax = 0;
			} else {
				sc->ebx = (seg << 3) | 7;
				sc->eax = ldt[seg].limit + 1;
			}
		}
	} else {
/* joerg OK: incr is negative! But why do we reduce mem of last_desc??? 
		while ((-incr > (ldt[last_desc].limit + 1)) && (last_desc > base_desc)) {
			incr += ldt[last_desc].limit + 1;
			last_desc--;
		}
		if(-incr > (ldt[last_desc].limit + 1)){
			last_desc = t_last;
			return -1;
		}

		seg = last_desc;
		sc->ebx = (seg << 3) | 7;
		sc->eax = ldt[seg].limit + incr + 1;
		if(sc->eax == 0){	*//* we have freed a segment *//*
			last_desc--;
			}
-joerg try this: */
/* -incr is less seg.limit so */
		sc->ebx = (seg << 3) | 7;
		sc->eax = ldt[seg].limit + incr + 1;
	}

	ldt[seg].limit += incr;

	d_print("x286emulm:   --------------------------------------------\n");
	pldt(); 

	/* Don't change the size of the near segment. It is always a
	 * full 64k because it has the stack in the top.
	 */
	if (seg > base_desc) {
		ldt_info.entry_number = seg;
		ldt_info.read_exec_only = 0;
		ldt_info.contents = LDT_DATA;
		ldt_info.seg_not_present = 0;
		ldt_info.useable = 1;
		ldt_info.seg_32bit = 0;
		ldt_info.limit_in_pages = 0;
		ldt_info.base_addr = ldt[seg].base;
		ldt_info.limit = ldt[seg].limit;
		d_print("x286emul:   segment %d: base = 0x%08lx, limit = 0x%04x\n",
		ldt_info.entry_number, ldt_info.base_addr, ldt_info.limit);
		modify_ldt(1, &ldt_info, sizeof(ldt_info));
	}
	/*
	 * Now get rid of any segments > last_desc
	 */
/* joerg 
	d_print("x286emulm: out: last_desc: %d t_last: %d\n", last_desc, t_last);
-joerg */
	while((t_last > last_desc) && (t_last > base_desc)){
		ldt_info.entry_number = t_last;
		ldt_info.read_exec_only = 1;
		ldt_info.contents = LDT_DATA;
		ldt_info.seg_not_present = 1;
		ldt_info.useable = 0;
		ldt_info.seg_32bit = 0;
		ldt_info.limit_in_pages = 0;
		ldt_info.base_addr = 0;
		ldt_info.limit = 0;
		d_print("x286emul:   segment %d: removed\n", ldt_info.entry_number);
		modify_ldt(1, &ldt_info, sizeof(ldt_info));
		t_last--;
	}
	return sc->eax + (sc->ebx << 16);
}

void pldt()
{
	int i = base_desc;
	d_print("x286emulm: seg\tbase\t\tlimit\t\trlimit\n");
	while (i <= last_desc)
	{
		d_print("x286emulm: %d\t0x%08lx\t0x%08lx\t0x%04x\n", i, ldt[i].base, ldt[i].limit, ldt[i].rlimit);
		i++;
	}
}
