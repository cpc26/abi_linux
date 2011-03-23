#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <alloca.h>

#include "x286emul.h"
#include "ldt.h"
#include "syscall.h"
#include "lcall7.h"
#include "debug.h"


int
emu_exec(struct sigcontext *sc)
{
	unsigned short *p;
	int argc, envc;
	char **q;
	unsigned short *stkladdr;

	if(LDATA){	/* Large Data */
		stkladdr = (unsigned short *)(ldt[sc->ss >> 3].base + (sc->ebx & 0xffff));
			check_addr(VERIFY_READ,stkladdr[1],stkladdr[0],1);
		sc->ebx = ldt[stkladdr[1]>>3].base + stkladdr[0];/* pgm */

			check_addr(VERIFY_READ,stkladdr[3],stkladdr[2],1);
		sc->ecx = ldt[stkladdr[3]>>3].base + stkladdr[2];/* arg */

			check_addr(VERIFY_READ,stkladdr[5],stkladdr[4],1);
		sc->esi = ldt[stkladdr[5]>>3].base + stkladdr[4];/* env */
	}
	else {
			check_addr(VERIFY_READ,sc->ds,sc->ebx & 0xffff,1);
		sc->ebx = ldt[sc->ds>>3].base + (sc->ebx & 0xffff);/* pgm */

			check_addr(VERIFY_READ,sc->ds,sc->ecx & 0xffff,1);
		sc->ecx = ldt[sc->ds>>3].base + (sc->ecx & 0xffff);/* arg */

			check_addr(VERIFY_READ,sc->ds,sc->esi & 0xffff,1);
		sc->esi = ldt[sc->ds>>3].base + (sc->esi & 0xffff);/* env */
	}


	d_print("x286emul:   exec: program = \"%s\"\n", (char *)sc->ebx);

	for (argc=0,p=(unsigned short *)sc->ecx; p && *p; p++,argc++) {
		if(LDATA){
			d_print("x286emul:         arg %d: \"%s\"\n",
				argc, (char *)ldt[*(p+1)>>3].base + *p);
				p++;
		}
		else
			d_print("x286emul:         arg %d: \"%s\"\n",
				argc, (char *)ldt[sc->ds>>3].base + *p);
	}

	for (envc=0,p=(unsigned short *)sc->esi; p && *p; p++,envc++) {
		if(LDATA){
			d_print("x286emul:         env %d: \"%s\"\n",
				envc, (char *)ldt[*(p+1)>>3].base + *p);
			p++;
		}
		else
			d_print("x286emul:         env %d: \"%s\"\n",
				envc, (char *)ldt[sc->ds>>3].base + *p);
	}

	if (!(q = alloca(sizeof(char *) * (argc + envc + 2)))) {
		xerrno = ENOMEM;
		return -1;
	}

	p = (unsigned short *)sc->ecx;
	sc->ecx = (int)q;
	for (; p && *p; p++){
		if(LDATA){
			*(q++) = (char *)(ldt[*(p+1)>>3].base + *p);
			p++;
			}
		else
			*(q++) = (char *)(ldt[sc->ds>>3].base + *p);
		}
	*(q++) = (char *)0;

	p = (unsigned short *)sc->esi;
	sc->esi = (int)q;
	for (; p && *p; p++){
		if(LDATA){
			*(q++) = (char *)(ldt[*(p+1)>>3].base + *p);
			p++;
			}
		else
			*(q++) = (char *)(ldt[sc->ds>>3].base + *p);
		}
	*(q++) = (char *)0;

	return lcall7(sc->eax & 0xffff, sc->ebx, sc->ecx, sc->esi);
}
