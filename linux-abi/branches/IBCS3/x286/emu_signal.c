/*
 *	emu_signal.c -- Signals for Xenix 286 emulator.
 *
 *	Copyright (C) 1998 Hulcote Electronics (Europe) Ltd.
 *			   David Bruce (David@Hulcote.com)
 *
 *		For Mog ( 1985 - 17/7/1998)
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * This works by taking advantage of the fact that [ 12]86 Xenix programs
 * save all registers on entry to a signal handling routine and then
 * restore registers and do an "iret" on exit.
 *
 * So all we do is put the flags, cs, ip on the stack and then return
 * to the signal handling routine. Then at the end of the routine it
 * restores all the registers and returns to the position where the
 * main program was interrupted.
 *
 * If we get an interrupt while we are in the emulator servicing a 
 * system call we patch the stack so that on exit from the emulator 
 * we return to the signal handling routine and that in turn
 * returns to the location of the system call in the program.
 *
 * Note:
 * We do not pass the signal no. to the signal handling routine 
 * because the routine works this out from the entry location.
 *
 * The main problem is that no signals are blocked while we are 
 * in the signal handling routine of the program. But then this
 * is the same as on 386 Xenix.
 */
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/unistd.h>

#include "../include/xout.h"
#include "x286emul.h"
#include "emu_signal.h"
#include "ldt.h"
#include "lcall7.h"
# include "debug.h"

#define XENSIGPOLL 20

static struct sig_ent {
	int flag;
	unsigned int cs;
	unsigned int ip;
	} sig_table[NSIG];

/*
 * This signal conversion table is different from
 * the one in iBCS. 
 * 1. to stop the programs from using SIGSEGV 
 *    because the emulator uses it.
 * 2. to remove a duplicate in the table.
 *    because this caused problems with one of
 *    my programs.
 */ 
int to_linux_sig[NSIG] = { 0,
			SIGHUP,
			SIGINT,
			SIGQUIT,
			SIGILL,
			SIGTRAP,
			SIGIOT,
			SIGXFSZ,	/* SIGEMT, */
			SIGFPE,
			SIGKILL,
			SIGBUS,
			SIGXCPU,	/* SIGSEGV */
			SIGVTALRM,	/* SIGSYS, */
			SIGPIPE,
			SIGALRM,
			SIGTERM,
			SIGUSR1,
			SIGUSR2,
			SIGCLD,
			SIGPWR,
			SIGPOLL,
			0,
			};

int to_xenix_sig[NSIG];

#define TO_LINUX(nr) (to_linux_sig[(nr)])
#define TO_XENIX(nr) (to_xenix_sig[(nr)])

unsigned int INT_STACK = 0;

sigset_t signo_pending;


/*
 * The main interrupt handling function
 */
void
sig_sigf(int nr, struct sigcontext sc)
{
volatile struct sigcontext *psc;

unsigned char *laddr;
unsigned short *stkladdr;

	((unsigned long *)(&nr))[-1] = (unsigned long) xigret;

#ifdef DEBUG_STACK
	unsigned int now_ss,now_sp;
	__asm__ volatile ("\tmovl $0,%%eax\n"
		"\tmov %%ss,%%ax\n"
		"\tmovl %%esp,%%ebx\n"
		: "=a" (now_ss),"=b" (now_sp)
		: );
	d_print("\nx286sigl: stack now -> 0x%02x:0x%04x (0x%08lx)!\n",
		now_ss,
		now_sp,
		ldt[now_ss >> 3].base + ((ldt[now_ss >> 3].base)?(now_sp & 0xffff):(now_sp)) );
#endif

	/* Look up the linear address for the segment:offset of the
	 * eip and stack.
	 */
	laddr = (unsigned char *)(ldt[sc.cs >> 3].base + sc.eip);
	stkladdr = (unsigned short *)(ldt[sc.ss >> 3].base
			+ ((ldt[sc.ss >> 3].base)?(sc.esp_at_signal & 0xffff):(sc.esp_at_signal)));

	d_print("x286emul: got signal %2d: will call 0x%02x:0x%04x (0x%08lx)\n",
			nr,
			sig_table[nr].cs,
			sig_table[nr].ip,
			ldt[sig_table[nr].cs >>3].base + sig_table[nr].ip);

	dump_state(laddr, stkladdr, &sc);
	/*
	 * are we in the emulator
	 */
	if(!ldt[sc.cs >> 3].base){
		d_print("x286emul: got signal while in emulator\n");
		/*
		 * simple just delay taking the signal
		 * until the end of this or the next 
		 * syscall.
		 */
		sigaddset(&signo_pending, nr);
		d_print("x286emul: after sigaddset\n");
		return;
	}
	else {
		d_print("x286emul: got signal while not in emulator\n");
		psc = &sc;
	}
	do_sig_pending(nr, psc);
}


/*
 * function to action delayed signals
 */
void
do_sig_pending(int nr, volatile struct sigcontext *sc)
{
unsigned short *progstkladdr;

	progstkladdr = (unsigned short *)(ldt[sc->ss >> 3].base + (sc->esp & 0xffff));

	progstkladdr[-1] = (unsigned short)(sc->eflags & 0xffff);	/* The Flags */
	progstkladdr[-2] = (unsigned short)(sc->cs & 0xffff);		/* The Code Seg Register */
	progstkladdr[-3] = (unsigned short)(sc->eip & 0xffff);		/* Return Location */

	/* Must not do this because the program works this out from the address we call */
	/* this would just corrupt the ax register */
	/*sc->eax = TO_XENIX(nr);					   set for return of signal # */
	sc->esp = (sc->esp - 6) & 0xffff;				/* allow for address on stack */
	sc->cs	= sig_table[nr].cs & 0xffff;				/* interrupt routine code segment */
	sc->eip = sig_table[nr].ip & 0xffff;				/* interrupt routine instruction pointer */

	/*
	 * Reset the interrupt table
	 */
	sig_table[nr].flag = 0;
	sig_table[nr].cs = 0;
	sig_table[nr].ip = 0;

}


/*
 * function to initialize tables etc.
 */
void
init_sigs()
{
int i;

	for(i=0;i<NSIG;i++){
		sig_table[i].flag = 0;
		sig_table[i].cs = 0;
		sig_table[i].ip = 0;

		to_xenix_sig[i] = 0;
		}

	for(i=0;i<NSIG;i++){
		if(TO_LINUX(i))
			to_xenix_sig[TO_LINUX(i)] = i;
		}

	INT_STACK = SEGV_STACK - 0x4000;

	sigemptyset(&signo_pending);
}


/*
 * The signal() system call
 */
int
emu_i_signal(struct sigcontext *sc)
{
int tflag,tcs,tip,result;
unsigned short signr,func_cs,func_add;
unsigned short *stkladdr;

	if(LDATA){	/* Large Data */
		stkladdr = (unsigned short *)(ldt[sc->ss >> 3].base + (sc->ebx & 0xffff));
		signr = stkladdr[0];
		func_add = stkladdr[1];
		func_cs = stkladdr[2];
	}
	else {
		signr = sc->ebx & 0xffff;
		func_add = sc->ecx & 0xffff;
		if(LTEXT){
			func_cs = sc->esi & 0xffff;
		}
		else {
			func_cs = sc->cs & 0xffff;
		}
	}

	if((signr) >= NSIG){
		sc->eax = EINVAL;
		if(LTEXT)
			sc->ebx = 0xff;
		sc->eflags |= 1;
		return -1;
	}
	signr = TO_LINUX(signr);

	tflag = sig_table[signr].flag;
	tcs = sig_table[signr].cs;
	tip = sig_table[signr].ip;

	if(func_add & 0xfffe){	/* address of a function */

		if(verify_area(VERIFY_EXEC, func_cs, func_add, 1)){
			sc->eax = EINVAL;
			if(LTEXT)
				sc->ebx = 0xff;
			sc->eflags |= 1;
			return -1;
		}

		/* Save address */
		sig_table[signr].flag = 1;
		sig_table[signr].cs = func_cs;
		sig_table[signr].ip = func_add;

		/* call function */
		{
			struct xigaction sa;
			result = -1;

			if(!xigaction(signr, NULL, &sa))
				result = (int)sa.handler;

			sa.handler = (__sighandler_t)sig_sigf;
			sa.mask  = 0xFFFFFFFF;
			sa.mask &= ~(1 << (SIGSEGV-1));
			sa.flags = SA_ONESHOT;

			sa.restorer = (void (*)(void)) INT_STACK;

			if(xigaction(signr, &sa, NULL))
				result = -1;
		}
	}
	else {
		/* Save the code */
		sig_table[signr].flag = func_add;
		sig_table[signr].cs = 0;
		sig_table[signr].ip = func_add;

		/* call function */
		{
			struct xigaction sa;
			result = -1;

			if(!xigaction(signr, NULL, &sa))
				result = (int)sa.handler;

			sa.handler = (__sighandler_t)((unsigned long)func_add);
			sa.mask = 0;
			sa.flags = 0;

			sa.restorer = (void (*)(void)) INT_STACK;

			if(xigaction(signr, &sa, NULL))
				result = -1;
		}
	}

	if(result == -1){
		sig_table[signr].flag = tflag;
		sig_table[signr].cs = tcs;
		sig_table[signr].ip = tip;
		sc->eax = xerrno;
		if(LTEXT)
			sc->ebx = 0xff;
		sc->eflags |= 1;
		return -1;
	}
	else {
		if(tflag){
			if(LTEXT)
				sc->ebx = tcs;
			sc->eax = tip;
		}
		else {
			if(LTEXT)
				sc->ebx = 0;
			sc->eax = 0;
			/*
			 * Look out for signals that were ignored before
			 * entering the program
			 */
			if(result == 1)
				sc->eax = 1;
		}
		sc->eflags &= ~1;
	}

	return 0;
}


/*
 * The kill() system call
 */
int
emu_i_kill(struct sigcontext *sc)
{
unsigned short *stkladdr;

	if(LDATA){	/* Large Data */
		stkladdr = (unsigned short *)(ldt[sc->ss >> 3].base + (sc->ebx & 0xffff));
		return kill( stkladdr[0], TO_LINUX(stkladdr[1]));
	}
	else {
		return kill( sc->ebx & 0xffff, TO_LINUX(sc->ecx & 0xffff));
	}
}
