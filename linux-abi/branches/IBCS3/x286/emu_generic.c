#include <errno.h>
#include <signal.h>
#include <stdio.h>

#include "x286emul.h"
#include "ldt.h"
#include "syscall.h"
#include "lcall7.h"
# include "debug.h"

int
emu_i_sas(struct sigcontext *sc)
{
unsigned short *stkladdr;

	if(LDATA){	/* Large Data */
		stkladdr = (unsigned short *)(ldt[sc->ss >> 3].base + (sc->ebx & 0xffff));
		check_addr (VERIFY_READ,stkladdr[2],stkladdr[1],1);
		return lcall7( sc->eax & 0xffff, 
			stkladdr[0],
			stkladdr[1] + ldt[stkladdr[2] >>3].base, 
			stkladdr[3]);
	}
	else {
		check_addr (VERIFY_READ,sc->ds,sc->ecx & 0xffff,1);
		return lcall7(sc->eax & 0xffff,
			sc->ebx & 0xffff,
			ldt[sc->ds>>3].base + (sc->ecx & 0xffff),
			sc->esi & 0xffff);
	}
}


int
emu_i_ass(struct sigcontext *sc)
{
unsigned short *stkladdr;

	if(LDATA){	/* Large Data */
		stkladdr = (unsigned short *)(ldt[sc->ss >> 3].base + (sc->ebx & 0xffff));
		check_addr (VERIFY_READ,stkladdr[1],stkladdr[0],1);
		d_print("x286emul: \"%s\"!\n",
				(char *)ldt[stkladdr[1]>>3].base + stkladdr[0]);
		return lcall7(sc->eax & 0xffff,
				stkladdr[0] + ldt[stkladdr[1] >>3].base, 
				stkladdr[2],
				stkladdr[3]);
	}
	else {
		check_addr (VERIFY_READ,sc->ds,sc->ebx & 0xffff,1);
		d_print("x286emul: \"%s\"!\n",
			(char *)ldt[sc->ds>>3].base + (sc->ebx & 0xffff));
		return lcall7(sc->eax & 0xffff,
			ldt[sc->ds>>3].base + (sc->ebx & 0xffff),
			(sc->ecx & 0xffff),
			(sc->esi & 0xffff));
	}
}


int
emu_i_s(struct sigcontext *sc)
{
unsigned short *stkladdr;

	if(LDATA){	/* Large Data */
		stkladdr = (unsigned short *)(ldt[sc->ss >> 3].base + (sc->ebx & 0xffff));
		return lcall7(sc->eax & 0xffff,
				stkladdr[0]);
	}
	else {
		return lcall7(sc->eax & 0xffff,
				sc->ebx & 0xffff);
	}
}


int
emu_i_sls(struct sigcontext *sc)
{
unsigned short *stkladdr;

	if(LDATA){	/* Large Data */
		stkladdr = (unsigned short *)(ldt[sc->ss >> 3].base + (sc->ebx & 0xffff));
		return lcall7(sc->eax & 0xffff,
				stkladdr[0],
				(long)stkladdr[1] | ((long)stkladdr[2] << 16),
				stkladdr[3]);
	}
	else {
		return lcall7(sc->eax & 0xffff,
				sc->ebx & 0xffff,
				(sc->ecx & 0xffff) | (sc->esi << 16),
				(sc->edi & 0xffff));
	}
}


int
emu_i_ssl(struct sigcontext *sc)
{
unsigned short *stkladdr;

	if(LDATA){	/* Large Data */
		stkladdr = (unsigned short *)(ldt[sc->ss >> 3].base + (sc->ebx & 0xffff));
		return lcall7(sc->eax & 0xffff,
				stkladdr[0],
				stkladdr[1],
				(long)stkladdr[2] | ((long)stkladdr[3] << 16));
	}
	else {
		return lcall7(sc->eax & 0xffff,
				sc->ebx & 0xffff,
				(sc->ecx & 0xffff),
				(sc->esi & 0xffff) | (sc->edi << 16));
	}
}

/* joerg special emu_i_ssl*/
int
emu_locking(struct sigcontext *sc)
{
unsigned short *stkladdr;
volatile int ret;
volatile long  len;
volatile unsigned short func, fd, mode, mymode;

	if(LDATA){	/* Large Data */
		stkladdr = (unsigned short *)(ldt[sc->ss >> 3].base + (sc->ebx & 0xffff));
		func 	= sc->eax & 0xffff;
		fd 		= stkladdr[0];
		mode	= stkladdr[1];
		len		= (long)stkladdr[2] | ((long)stkladdr[3] << 16);
	}
	else {
		func 	= (sc->eax & 0xffff);
		fd		= (sc->ebx & 0xffff);
		mode	= (sc->ecx & 0xffff);
		len 	= ((sc->esi & 0xffff) | (sc->edi << 16));
	}
	if (V3LOCK) {
		/* a V3 (don't know about V2) binary will not get a lock for a region 
		   where someone else holds a lock independent of the types of locks 
		   althoug read/write access depends on the type of lock other processes
		   may have */
		/* some V3 programs rely on this implementation :-( */
		if (mode == 3 || mode == 4)	 {	/* ReadLocks */
			/* try to get a WriteLock */
			mymode = 2;					/* nonblocking WriteLock */
			ret = lcall7(func, fd, mymode, len); 
			if (ret == -1) {
				/* errno is 11 now but V3 gives 13 in this case */
				xerrno = 13;
				return(ret);
			}
			else {
				/* we got it, so apply the lock the user wants */
				return lcall7(func, fd, mode, len); 
			}
		}
		else {
			ret = lcall7(func, fd, mode, len); 
			if (ret == -1) {
				/* errno is 11 now but V3 gives 13 in this case */
				xerrno = 13;
				return(ret);
			}
			else
				return(ret);
		}
	
	}
	else
		return lcall7(func, fd, mode, len); 
}
/*- joerg */

int
emu_i_a(struct sigcontext *sc)
{
unsigned short *stkladdr;

	if(LDATA){	/* Large Data */
		stkladdr = (unsigned short *)(ldt[sc->ss >> 3].base + (sc->ebx & 0xffff));
		check_addr (VERIFY_READ,stkladdr[1],stkladdr[0],1);
		return lcall7(sc->eax & 0xffff,
				ldt[stkladdr[1]>>3].base + stkladdr[0]);
	}
	else {
		check_addr (VERIFY_READ,sc->ds,sc->ebx & 0xffff,1);
		return lcall7(sc->eax & 0xffff,
				ldt[sc->ds>>3].base + (sc->ebx & 0xffff));
	}
}

int
emu_i_aa(struct sigcontext *sc)
{
unsigned short *stkladdr;

	if(LDATA){	/* Large Data */
		stkladdr = (unsigned short *)(ldt[sc->ss >> 3].base + (sc->ebx & 0xffff));
		check_addr (VERIFY_READ,stkladdr[1],stkladdr[0],1);
		check_addr (VERIFY_READ,stkladdr[3],stkladdr[2],1);
		return lcall7(sc->eax & 0xffff,
				ldt[stkladdr[1]>>3].base + stkladdr[0],
				ldt[stkladdr[3]>>3].base + stkladdr[2]);
	}
	else {
		check_addr (VERIFY_READ,sc->ds,sc->ebx & 0xffff,1);
		check_addr (VERIFY_READ,sc->ds,sc->ecx & 0xffff,1);
		return lcall7(sc->eax & 0xffff,
				ldt[sc->ds>>3].base + (sc->ebx & 0xffff),
				ldt[sc->ds>>3].base + (sc->ecx & 0xffff));
	}
}

/* joerg special emu_i_a*/
struct xnx_utimbuf {
	long atime;
	long mtime;
};

int
emu_i_ssa(struct sigcontext *sc)
{
unsigned short *stkladdr;

	if(LDATA){	/* Large Data */
		stkladdr = (unsigned short *)(ldt[sc->ss >> 3].base + (sc->ebx & 0xffff));
		check_addr (VERIFY_READ,stkladdr[3],stkladdr[2],1);
		return lcall7(sc->eax & 0xffff,
			stkladdr[0],
			stkladdr[1],
			ldt[stkladdr[3]>>3].base + stkladdr[2]);
	}
	else {
		check_addr (VERIFY_READ,sc->ds,sc->esi & 0xffff,1);
		return lcall7(sc->eax & 0xffff,
			(sc->ebx & 0xffff),
			(sc->ecx & 0xffff),
			ldt[sc->ds>>3].base + (sc->esi & 0xffff));
	}
}


int
emu_i_l(struct sigcontext *sc)
{
unsigned short *stkladdr;

	if(LDATA){	/* Large Data */
		stkladdr = (unsigned short *)(ldt[sc->ss >> 3].base + (sc->ebx & 0xffff));
		return lcall7(sc->eax & 0xffff,
			stkladdr[0] | ((long)stkladdr[1] << 16));
	}
	else {
		return lcall7(sc->eax & 0xffff,
			(sc->ebx & 0xffff) | ((sc->ecx & 0xffff) << 16));
	}
}


int
emu_i_v(struct sigcontext *sc)
{
	return lcall7(sc->eax & 0xffff);
}
