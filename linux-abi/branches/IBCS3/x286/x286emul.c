#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/resource.h>
#include <linux/unistd.h>

#include "../include/xout.h"
#include "x286emul.h"
#include "emu_signal.h"
#include "ldt.h"
#include "debug.h"

struct xexec *xexec;
struct xext *xext;
int xerrno;

#ifdef DEBUG
FILE *__dbf = NULL;
#endif

unsigned long SEGV_STACK;
int LDATA,LTEXT,V3LOCK; 


#ifdef DEBUG_CALL
void
dump_state(unsigned char *laddr, unsigned short *stkladdr,
	struct sigcontext *sc)
{
	int i, zerrno;

	d_print("Trap 0x%08lx, error 0x%08lx\n", sc->trapno, sc->err);
	d_print("EIP: 0x%02x:0x%08lx (0x%08lx)\n",
		sc->cs, sc->eip, (unsigned long)laddr);
	d_print("STK: 0x%02x:0x%08lx (0x%08lx) BP: 0x%08lx\n",
		sc->ss, sc->esp_at_signal, (unsigned long)stkladdr, sc->ebp);
	d_print("EAX: 0x%08lx  EBX: 0x%08lx  ECX: 0x%08lx  EDX: 0x%08lx\n",
		sc->eax, sc->ebx, sc->ecx, sc->edx);
	d_print("ESI: 0x%08lx  EDI: 0x%08lx\n", sc->esi, sc->edi);
	d_print("DS:  0x%02x	ES:  0x%02x	SS:  0x%02x	FS:  0x%02x	GS:  0x%02x\n",
		sc->ds, sc->es, sc->ss, sc->fs, sc->gs);
	zerrno = xerrno;
	d_print("STACK:");
	if (verify_area(VERIFY_READ,sc->ss,(sc->esp_at_signal & 0xffff)-16,32) == 0) {
		for (i=-8; i<0; i++) d_print(" %04x", stkladdr[i]);
		d_print(" |");
		for (i=0; i<8; i++) d_print(" %04x", stkladdr[i]);
	}
	d_print("\n");
	d_print("CODE: ");
	if (verify_area(VERIFY_READ,sc->cs,sc->eip - 16,32) == 0) {
		for (i=-8; i<0; i++) d_print(" %02x", laddr[i]);
		d_print(" |");
		for (i=0; i<8; i++) d_print(" %02x", laddr[i]);
	}
	d_print("\n");
	xerrno = zerrno;
}
#endif

/* Libc sigaction buggers about with sa_restorer. */
int
xigaction(int sig, struct xigaction *new, struct xigaction *old)
{
	__asm__("int $0x80"
		: "=a" (sig)
		: "0" (__NR_sigaction), "b" (sig), "c" (new), "d" (old));
	if (sig >= 0)
		return 0;
	xerrno = -sig;
	return -1;
}


void
trap_signal(int signr, __sighandler_t func, int flags)
{
	struct xigaction sa;

	sa.handler = func;
	sa.mask  = 0xFFFFFFFF;
	sa.mask &= ~(1 << (signr-1));
	sa.mask &= ~(1 << (SIGSEGV-1));
	sa.flags = flags;
	sa.restorer = (void (*)(void)) SEGV_STACK;
	xigaction(signr, &sa, NULL);
}


/*
 * Routine to replace floating point emulator calls
 * with the real floating point op codes.
 * This info comes via a nice man at SCO.
 * Note 'F8' and 'FA' are untested, i have never
 * seen a program use them.
 */
void
fpfixup(unsigned char *laddr)
{
	
	laddr[0] = 0x9b;
	switch (laddr[1] ){
		case 0xf8:
			laddr[1] = 0x26;
			laddr[2] = 0xd8 | (laddr[2] & 0x07);
			break;
		case 0xf9:
			laddr[1] = 0x90;
			break;
		case 0xfa:
			laddr[1] = 0x36;
			laddr[2] = 0xd8 | (laddr[2] & 0x07);
			break;
		default:
			laddr[1] = 0xd8 | (laddr[1] & 0x07);
		}
}


void
sig_segv(int nr, struct sigcontext sc)
{
	unsigned char *laddr;
	unsigned short *stkladdr;
	static int sig_segv_count = 0;

	((unsigned long *)(&nr))[-1] = (unsigned long) xigret;
	if(sig_segv_count++ > 4)
		exit(1);

#ifdef DEBUG_STACK
	unsigned int s,p;
	__asm__ volatile ("\tmovl $0,%%eax\n"
		"\tmov %%ss,%%ax\n"
		"\tmovl %%esp,%%ebx\n"
		: "=a" (s),"=b" (p)
		: );
	d_print("\nx286segv: stack now -> 0x%02x:0x%04x (0x%08lx)!\n",
			s,
			p,
			ldt[s >> 3].base + ((ldt[s >> 3].base)?(p & 0xffff):(p)));
#endif

	/* Look up the linear address for the segment:offset of the
	 * eip and stack.
	 */
	laddr = (unsigned char *)(ldt[sc.cs >> 3].base + sc.eip);
	stkladdr = (unsigned short *)(ldt[sc.ss >> 3].base
			+ ((ldt[sc.ss >> 3].base)?(sc.esp_at_signal & 0xffff):(sc.esp_at_signal)));

	dump_state(laddr, stkladdr, &sc);

	if (ldt[sc.cs >> 3].base) {	/* Not in the emulator */
		if (laddr[-1] == 0x9d && laddr[0] == 0xff && laddr[1] == 0x26) {
			d_print("x286emul: Interrupt return problem??\n");
		}
		if (laddr[0] == 0xcc){
			d_print("x286emul: Int 0x03 at 0x%02x:0x%04lx\n",sc.cs,sc.eip);
		}
		if (laddr[0] == 0xcd){
			d_print("x286emul: Int 0x%02x at 0x%02x:0x%04lx\n",laddr[1],sc.cs,sc.eip);
			if ((laddr[1] >= 0xf0) && (laddr[1] <= 0xfa)){
				fpfixup(laddr);
				sig_segv_count = 0;
				return;
			}
		}

	}

	fprintf(stderr, "x286emul: segv%s!\n", !ldt[sc.cs >> 3].base ? " in emulator" : "");

	d_print("x286emul: segv%s!\n",
		!ldt[sc.cs >> 3].base ? " in emulator" : "");
	dump_state(laddr, stkladdr, &sc);

	exit(1);
}


/*
 * Patch the first 128 bytes of the x286 program to call
 * the emulator on a syscall
 */
unsigned char head_data[] = {	0xeb, 0x14,		/* jmp 0x18 */
				0xeb, 0x0e,		/* jmp 0x14 */
				0xeb, 0x0c,		/* jmp 0x14 */
				0xeb, 0x19,		/* jmp 0x20 */
				0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00,
				0xb8, 0x28, 0x08,	/* mov 0x2808,ax */
				0x90,			/* nop */
				0x90,			/* nop */
				0x66,
				0x9a, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00,
				0xeb, 0x5c,		/* jmp 0x7f */
				0xf4,			/* hlt */
				0xff, 0xff };
void
fix_header(void)
{
	unsigned char *address;
	int i;
	int t_cs;
	unsigned short *aloc;
	void __x286syscall(void);

	address = (unsigned char *)(ldt[init_cs >> 3].base + init_entry +2);
	for (i=0; head_data[i] < 0xff; i++)
		address[i] = head_data[i];

	/* now patch in the address of "__x286syscall()" */
	aloc = (unsigned short *)(ldt[init_cs >> 3].base + init_entry + 0x1b);

	__asm__ volatile ("\tsubl %%eax,%%eax\n"
		"\tmov %%cs,%%ax\n"
		: "=a" (t_cs)
		: );

	aloc[0] = ((unsigned long)__x286syscall) & 0xFFFF;
	aloc[1] = ((unsigned long)__x286syscall) >> 16;
	aloc[2] = (unsigned short)t_cs;
}


static unsigned short
set_frame(unsigned long base, unsigned long offset, char *argv[], char *envp[])
{
	char *ap, *ep, *sp;
	int envc, argc, i;

	sp = (char *)(base + offset);

	for (envc=0; envp[envc]; envc++) {
		sp -= (strlen(envp[envc])+2) & ~1;
#ifdef DEBUG_ENV
		d_print("string: 0x%04lx (0x%08lx): \"%s\"\n",
			(unsigned long)(sp-base), (unsigned long)sp, envp[envc]);
#endif
		strcpy(sp, envp[envc]);
	}
	ep = sp;

	for (argc=0; argv[argc]; argc++) {
		sp -= strlen(argv[argc])+1;
#ifdef DEBUG_ENV
		d_print("string: 0x%04lx (0x%08lx): \"%s\"\n",
				(unsigned long)(sp-base), (unsigned long)sp, argv[argc]);
#endif
		strcpy(sp, argv[argc]);
	}
	ap = sp;

	*(--sp) = 0; *(--sp) = 0;
	*(--sp) = 0; *(--sp) = 0;
	if((int)sp & 1)
		*(--sp) = 0;
#ifdef DEBUG_ENV
	d_print("array:  0: 0x%04lx (0x%08lx) -> NULL\n",
		(unsigned long)(sp-base), (unsigned long)sp);
#endif
	for (i=envc; i--;) {
		sp -= 2;
		if(LDATA){
			*((unsigned short *)sp) = init_ds;
			sp -= 2;
			}
#ifdef DEBUG_ENV
		d_print("array: %2d: 0x%04lx (0x%08lx) -> 0x%04lx (0x%08lx)\n",
				i+1,
				(unsigned long)(sp-base), (unsigned long)sp,
				(unsigned long)(ep-base), (unsigned long)ep);
#endif
		*((unsigned short *)sp) = (unsigned long)(ep - base);
		while (*ep++);
		ep = (char *)((long)++ep & ~1);
	}

	if(LDATA){
		*(--sp) = 0; *(--sp) = 0;
		}
	*(--sp) = 0; *(--sp) = 0;
#ifdef DEBUG_ENV
	d_print("array:  0: 0x%04lx (0x%08lx) -> NULL\n",
		(unsigned long)(sp-base), (unsigned long)sp);
#endif

	for (i=argc; i--;) {
		sp -= 2;
		if(LDATA){
			*((unsigned short *)sp) = init_ds;
			sp -= 2;
			}
#ifdef DEBUG_ENV
		d_print("array: %2d: 0x%04lx (0x%08lx) -> 0x%04lx (0x%08lx)\n",
				i+1,
				(unsigned long)(sp-base), (unsigned long)sp,
				(unsigned long)(ap-base), (unsigned long)ap);
#endif
		*((unsigned short *)sp) = (unsigned long)(ap - base);
		while (*ap++);
	}

	sp -= 2;
	*((unsigned short *)sp) = argc;

	return (unsigned long)(sp-base);
}


int
main(int argc, char *argv[], char *envp[])
{
	int x286boot(unsigned long, unsigned long);
	unsigned long ds_base;
	void load_xout(char *);
	char *p;

#ifdef DEBUG
	int fd,fd1;
	char p1[256];

	if ((p = getenv("X286DEBUG")) && (getuid() == geteuid())){
		snprintf(p1,255,p,getpid());
		fd = open(p1,O_CREAT | O_APPEND | O_WRONLY, 0644);
		if(fd > 0){
			fd1 = dup2(fd,0x3f);
			close(fd);
			__dbf = fdopen(fd1, "a");
			if(__dbf) setbuf(__dbf, 0);
		}
	}
#endif
	load_xout(argv[0]);
	d_print("Xenix 286 emulation: entry 0x%02lx:0x%04lx, data 0x%02lx, stack 0x%02lx:0x%04lx\n",
		init_cs, init_entry, init_ds, init_ss, limit_stk);

	/* If the stack size isn't already specified in the header set
	 * it to the Xenix default of 0x1000. We need to know this at run
	 * time in order to implement the stkgro check.
	 */
	if (!(xexec->x_renv & XE_FS))
		xext->xe_stksize = 0x1000;

	LDATA = xexec->x_renv & (XE_LDATA | XE_HDATA);
	LTEXT = xexec->x_renv & XE_LTEXT;
/*-joerg */
	if ((xexec->x_renv & XE_V3) && !(xexec->x_renv & XE_V2))
		V3LOCK = 1;
	else
		V3LOCK = 0;

	d_print("Lock handling for V3 turned %s\n", (V3LOCK ? "on" : "off"));
/* joerg-*/

	ldt_init();
	base_desc = init_ds >> 3;

	ds_base = (unsigned long)ldt[base_desc].base;
	init_stk = set_frame(ds_base, 0x10000, argv, envp);

	ldt[base_desc].limit = limit_stk | 1;
	ldt[base_desc].rlimit = init_stk - xext->xe_stksize - 1;
	if(ldt[base_desc].rlimit < ldt[base_desc].limit){
		fprintf(stderr, "x286emul: No Stack Space\n");
		exit(1);
		}

	fix_header();

	d_print("stack addr = 0x%04lx\n", init_stk);

	{
	unsigned int s,p;
	__asm__ volatile ("\tmovl $0,%%eax\n"
		"\tmov %%ss,%%ax\n"
		"\tmovl %%esp,%%ebx\n"
		: "=a" (s),"=b" (p)
		: );
	SEGV_STACK = (p - 0x4000) & ~7;

#ifdef DEBUG_STACK
	d_print("x286init: stack now -> 0x%02x:0x%04x (0x%08lx) SEGV_STACK=0x%08lx\n",
		s,
		p,
		ldt[s >> 3].base + ((ldt[s >> 3].base)?(p & 0xffff):(p)) ,SEGV_STACK);
#endif
	}

	init_sigs(); /* ONLY AFTER SEGV_STACK INITIALIZED */

	trap_signal(SIGSEGV, (__sighandler_t)sig_segv, SA_NOMASK);
	
	return x286boot((unsigned long)ldt[last_desc].limit,(unsigned long)(last_desc << 3) | 7);
}
