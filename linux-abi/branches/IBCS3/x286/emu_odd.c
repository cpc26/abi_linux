/*
 *	emu_odd.c -- Odd syscall's and fixes.
 *
 *	Copyright (C) 1998 Hulcote Electronics (Europe) Ltd.
 *			   David Bruce (David@Hulcote.com)
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
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "x286emul.h"
#include "ldt.h"
#include "syscall.h"
#include "lcall7.h"
#include "debug.h"
extern void pldt(void);

struct xnx_timeb {
	long		time;
	unsigned short	millitm;
	short		timezone;
	short		dstflg;
	};
#define SIZE_TIMEB 10

struct xnx_flock {
	short	l_time;
	short	l_whence;
	long	l_start;
	long	l_len;
	short	l_pid;
	short	l_sysid;
	};
#define SIZE_FLOCK 16

#define SIZE_STAT 30


/*
 * for syscalls that return a second parameter in dx
 * But donot pass any parameters to the system.
 * We define the output as a long so that the two
 * parameters get passed back to the 286 program.
 * used by pipe() and wait().
 */
int
emu_i_pipe(struct sigcontext *sc)
{
	register int res;

	__asm__ volatile ("\n"
			".byte\t0x9a,0,0,0,0,7,0\n"
			"\tjnc Lexit\n"
			"\tmovl %%eax,xerrno\n"
			"\tmovl $-1,%%eax\n"
			"\tmovl %%eax,%%edx\n"
			"Lexit:\n\t"
			: "=a" (res) , "=d" (sc->ebx)
			: "0" (sc->eax & 0xffff));

	return (res & 0xffff) | (sc->ebx << 16);

}


/*
 * structure is a multiple of 4 bytes so
 * no problem with the size of the struct but two
 * entry's in the struct are swapped 286 -> 386
 */
int
emu_i_fcntl(struct sigcontext *sc)
{
unsigned short *stkladdr;
struct xnx_flock *fl;
int result,t;
unsigned short filedes, cmd, arg, arg_seg;

	if(LDATA){	/* Large Data */
		stkladdr = (unsigned short *)(ldt[sc->ss >> 3].base + (sc->ebx & 0xffff));
		filedes = stkladdr[0];
		cmd = stkladdr[1];
		arg = stkladdr[2];
		arg_seg = stkladdr[3];
	} else {
		filedes = sc->ebx & 0xffff;
		cmd = sc->ecx & 0xffff;
		arg = sc->esi & 0xffff;
		arg_seg = sc->ds ;
	}

	if(cmd >= 5 && cmd <= 7) {	/* arg 3 a pointer */

		
		check_addr(VERIFY_WRITE, arg_seg, arg, SIZE_FLOCK);
		if(result) {
			xerrno = result;
			return -1;
		}

		fl = (struct xnx_flock *)(ldt[arg_seg>>3].base + arg);

		result = lcall7(sc->eax & 0xffff,
				filedes,
				cmd,
				ldt[arg_seg>>3].base + arg);

		if((cmd == 5) && (result >= 0)) {
			/* Swap pid and sysid fields */
			t = fl->l_pid;
			fl->l_pid = fl->l_sysid;
			fl->l_sysid = t;
		}

		return result ;
	} else {				/* arg 3 a short */
		return lcall7(sc->eax & 0xffff,
			filedes,
			cmd,
			arg);
	}
}


struct ibcs_stat {
        unsigned short st_dev;
        unsigned short st_ino;
        unsigned short st_mode;
        unsigned short st_nlink;
        unsigned short st_uid;
        unsigned short st_gid;
        unsigned short st_rdev;
        unsigned long  st_size;
        unsigned long  st_atime;
        unsigned long  st_mtime;
        unsigned long  st_ctime;
};

/*
 * fstat() system call with fixes for the size 
 * of the structure and the alignment of longs
 * in the structure.
 */
int
emu_i_fstat(struct sigcontext *sc)
{
unsigned short *stkladdr;
int result;
struct ibcs_stat td;
unsigned short filedes, buf, buf_seg;

	if(LDATA){	/* Large Data */
		stkladdr = (unsigned short *)(ldt[sc->ss >> 3].base + (sc->ebx & 0xffff));
		filedes = stkladdr[0];
		buf = stkladdr[1];
		buf_seg = stkladdr[2];
	} else {
		filedes = sc->ebx & 0xffff;
		buf = sc->ecx & 0xffff;
		buf_seg = sc->ds ;
	}

	check_addr(VERIFY_WRITE, buf_seg, buf, SIZE_STAT);

	result = lcall7( sc->eax & 0xffff, filedes, &td );
	if(result >= 0){
		memmove((void *)(&td)+14,(void *)(&td)+16,16);
		memcpy((void *)(buf + ldt[buf_seg >>3].base), (void *)&td, SIZE_STAT);
	}
	return result;
}


/*
 * stat() system call with fixes for the size 
 * of the structure and the alignment of longs
 * in the structure.
 */
int
emu_i_stat(struct sigcontext *sc)
{
unsigned short *stkladdr;
int result;
struct ibcs_stat td;
unsigned short path, path_seg, buf, buf_seg;

	if(LDATA){	/* Large Data */
		stkladdr = (unsigned short *)(ldt[sc->ss >> 3].base + (sc->ebx & 0xffff));
		path = stkladdr[0];
		path_seg = stkladdr[1];
		buf = stkladdr[2];
		buf_seg = stkladdr[3];
	} else {
		path = sc->ebx & 0xffff;
		path_seg = sc->ds;
		buf = sc->ecx & 0xffff;
		buf_seg = sc->ds ;
	}

	check_addr(VERIFY_READ, path_seg, path, 1);
	check_addr(VERIFY_WRITE, buf_seg, buf, SIZE_STAT);

	result = lcall7( sc->eax & 0xffff, ldt[path_seg >> 3].base + path, &td );
	if(result >= 0){
		memmove((void *)(&td)+14,(void *)(&td)+16,16);
		memcpy((void *)(buf + ldt[buf_seg >>3].base), (void *)&td, SIZE_STAT);
	}
	return result;
}


/*
 * ftime() system call with fixes for the size 
 * of the structure.
 */
int
emu_i_ftime(struct sigcontext *sc)
{
unsigned short *stkladdr;
int result;
struct xnx_timeb td;
unsigned short tp, tp_seg;

	d_print("ebx: 0x%08lx ds: 0x%04x LDATA: %d\n", sc->ebx, sc->ds, LDATA);
	pldt();

	if(LDATA){	/* Large Data */
		stkladdr = (unsigned short *)(ldt[sc->ss >> 3].base + (sc->ebx & 0xffff));
		tp = stkladdr[0];
		tp_seg = stkladdr[1];
	} else {
		tp = sc->ebx & 0xffff;
		tp_seg = sc->ds;
	}

	check_addr(VERIFY_WRITE, tp_seg, tp, SIZE_TIMEB);

	result = lcall7(sc->eax & 0xffff, &td );
	memcpy((void *)(tp + ldt[tp_seg >>3].base), (void *)&td, SIZE_TIMEB);

	d_print("tp: %d tp_seg %d base: 0x%08lx\n", tp, tp_seg, ldt[tp_seg >>3].base);
	pldt();
	d_print("------------------------------\n");

	return result;
}


#define NCC 8
struct xnx_termio {
	unsigned short c_iflag;
	unsigned short c_oflag;
	unsigned short c_cflag;
	unsigned short c_lflag;
	char c_line;
	unsigned char c_cc[NCC];
};
#define SIZE_TERMIO 17

/*
 * ioctl() system call
 * Big problems with this catch all system call
 * where the parameters can be ints or pointers
 * of various sizes pointing to data of various 
 * sizes.
 * "TERMIO" is a start but anything else is DIY.
 */
int
emu_i_ioctl(struct sigcontext *sc)
{
unsigned short *stkladdr;
unsigned short op, fd, type, arg, seg;
int result;
struct xnx_termio td;

	d_print("ebx: 0x%08lx ds: 0x%04x LDATA: %d\n", sc->ebx, sc->ds, LDATA);
	pldt();
	d_print("------------------------------\n");

	if(LDATA){	/* Large Data */
		stkladdr = (unsigned short *)(ldt[sc->ss >> 3].base + (sc->ebx & 0xffff));
		op = sc->eax & 0xffff;
		fd = stkladdr[0];
		type = stkladdr[1];
		arg = stkladdr[2];
		seg = stkladdr[3];
	} else {
		op = sc->eax & 0xffff;
		fd = sc->ebx & 0xffff;
		type = sc->ecx & 0xffff;
		arg = sc->esi & 0xffff;
		seg = sc->ds;
	}

	if((type >> 8) == 'T'){	/* standard TERMIO call */
		if((type & 0xff) <= 4){
			check_addr((type & 0xff)==1 ? VERIFY_WRITE:VERIFY_READ,seg,arg,SIZE_TERMIO);
			if((type & 0xff) != 1)
				memcpy((void *)&td, (void *)(ldt[seg>>3].base + arg), SIZE_TERMIO);
			d_print("op: 0x%04x fd: 0x%04x type: 0x%04x \n", op, fd, type);
			result = lcall7(op, fd, type, &td);

			d_print("--------5---------------------\n");

			if((result >= 0) && ((type & 0xff) == 1))
				memcpy((void *)(ldt[seg>>3].base + arg), (void *)&td, SIZE_TERMIO);
			return result;
		} else {
			d_print("--------6---------------------\n");
			return lcall7(op,fd,type,arg);
		}
	} else {
		/*
		 * Not setup for this type
		 * so rather than guess return an error
		 */
		xerrno = EINVAL;
		return -1;
	}
}


/*
 * dup() system call
 * (should/is) this be in iBCS/Linux?
 */
int
emu_i_dup(struct sigcontext *sc)
{
unsigned short *stkladdr;
int fd1,fd2;

	if(LDATA){	/* Large Data */
		stkladdr = (unsigned short *)(ldt[sc->ss >> 3].base + (sc->ebx & 0xffff));
		fd1 = stkladdr[0];
		fd2 = stkladdr[1];
	} else {
		fd1 = sc->ebx & 0xffff;
		fd2 = sc->ecx & 0xffff;
	}

	if (fd1 & 0x40) {
		return dup2(fd1 & 0x3f,fd2);
	} else {
		return dup(fd1);
	}
}
/*
 * ulimit() system call
 *
 */

int
emu_i_ulimit(struct sigcontext *sc)
{
unsigned short *stkladdr = 0;
int cmd,arg = 0;
#define MEMLIM (0xdfffff)	/* tell them they can have 14 megs of data */

	if(LDATA){	/* Large Data */
		stkladdr = (unsigned short *)(ldt[sc->ss >> 3].base + (sc->ebx & 0xffff));
		cmd = stkladdr[0];
	} else {
		cmd = sc->ebx & 0xffff;
	}

	switch(cmd) {
		case 2:		/* set file limit */
			if(LDATA){	/* Large Data */
				arg = (long)stkladdr[1] | ((long)stkladdr[2] << 16);
			} else {
				arg = (sc->ecx & 0xffff) | (sc->esi << 16);
			}
		case 1:		/* get file limit */
		case 4:		/* max files */
			return lcall7(sc->eax & 0xffff, cmd, arg);

		case 3:
			if(LDATA){	/* Large Data */
				arg = lcall7(sc->eax & 0xffff, cmd, arg);
				if(arg > MEMLIM)
					arg = MEMLIM;
				return arg;
			} else {
				return ldt[last_desc].rlimit;
			}

		case 64:	/* get text offset */
			if(LTEXT){
				if(LDATA)
					return (long)stkladdr[1] + ldt[stkladdr[2] >> 3].base -
						ldt[init_cs >> 3].base;
				return (sc->ecx & 0xffff) + ldt[(sc->esi &0xffff) >> 3].base -
					ldt[init_cs >> 3].base;
			} else {
				if(LDATA)
					return (long)stkladdr[1];
				return sc->ecx & 0xffff;
			}
		default:
			xerrno = EINVAL;
			return -1;
	}
}

