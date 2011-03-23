#include <errno.h>
#include <signal.h>
#include <stdio.h>

#include "x286emul.h"
#include "ldt.h"
#include "syscall.h"
#include "emu_signal.h"
# include "debug.h"

#ifdef DEBUG
# define NAME(X)	X,
#else
# define NAME(X)
#endif


#define R_VOID		0
#define R_SPECIAL	1
#define R_POINTER	2
#define R_SHORT		3
#define R_LONG		4
#define R_FPOINTER	5


typedef int (*syscall_func_t)(struct sigcontext *);

typedef struct syscall {
#ifdef DEBUG
	char *name;
#endif
	syscall_func_t func;
	int ret_type;
} syscall_t;


static syscall_t unix_syscall[] = {					  /* T == tried */
	{ NAME("null")		(syscall_func_t)0	,R_SHORT	}, /*	0	*/
	{ NAME("exit")		emu_i_s			,R_VOID		}, /*	1 T	*/
	{ NAME("fork")		emu_i_v			,R_SHORT	}, /*	2 T	*/
	{ NAME("read")		emu_i_sas		,R_SHORT	}, /*	3 T	*/
	{ NAME("write")		emu_i_sas		,R_SHORT	}, /*	4 T	*/
	{ NAME("open")		emu_i_ass		,R_SHORT	}, /*	5 T	*/
	{ NAME("close")		emu_i_s			,R_SHORT	}, /*	6 T	*/
	{ NAME("wait")		emu_i_pipe		,R_LONG		}, /*	7 T	*/
	{ NAME("creat")		emu_i_ass		,R_SHORT	}, /*	8 T	*/
	{ NAME("link")		emu_i_aa		,R_SHORT	}, /*	9	*/
	{ NAME("unlink")	emu_i_a			,R_SHORT	}, /*	10 T	*/
	{ NAME("exec")		emu_exec		,R_SHORT	}, /*	11 T	*/
	{ NAME("chdir")		emu_i_a			,R_SHORT	}, /*	12 T	*/
	{ NAME("time")		emu_i_a			,R_LONG		}, /*	13 T	*/
	{ NAME("mknod")		emu_i_ass		,R_SHORT	}, /*	14	*/
	{ NAME("chmod")		emu_i_ass		,R_SHORT	}, /*	15	*/
	{ NAME("chown")		emu_i_ass		,R_SHORT	}, /*	16	*/
	{ NAME("brk/break")	emu_brk			,R_SHORT	}, /*	17	*/
	{ NAME("stat")		emu_i_stat		,R_SHORT	}, /*	18 T	*/
	{ NAME("seek/lseek")	emu_i_sls		,R_LONG		}, /*	19 T	*/
	{ NAME("getpid")	emu_i_s			,R_SHORT	}, /*	20 T	*/
	{ NAME("mount")		(syscall_func_t)0	,R_SHORT	}, /*	21	*/
	{ NAME("umount")	(syscall_func_t)0	,R_SHORT	}, /*	22	*/
	{ NAME("setuid")	emu_i_s			,R_SHORT	}, /*	23 T	*/
	{ NAME("getuid")	emu_i_s			,R_SHORT	}, /*	24 T	*/
	{ NAME("stime")		emu_i_l			,R_SHORT	}, /*	25	*/
	{ NAME("ptrace")	(syscall_func_t)0	,R_SHORT	}, /*	26	*/
	{ NAME("alarm")		emu_i_s			,R_SHORT	}, /*	27 T	*/
	{ NAME("fstat")		emu_i_fstat		,R_SHORT	}, /*	28 T	*/
	{ NAME("pause")		emu_i_v			,R_SHORT	}, /*	29 T	*/
	{ NAME("utime")		emu_i_aa		,R_SHORT	}, /* 	30	*/
	{ NAME("stty")		(syscall_func_t)0	,R_SHORT	}, /*	31	*/
	{ NAME("gtty")		(syscall_func_t)0	,R_SHORT	}, /*	32	*/
	{ NAME("access")	emu_i_ass		,R_SHORT	}, /*	33 T	*/
	{ NAME("nice")		emu_i_s			,R_SHORT	}, /*	34	*/
	{ NAME("statfs/ftime")	(syscall_func_t)0	,R_SHORT	}, /*	35	*/
	{ NAME("sync")		emu_i_v			,R_VOID		}, /*	36	*/
	{ NAME("kill")		emu_i_kill		,R_SHORT	}, /*	37 T	*/
	{ NAME("fstatfs/clocal") (syscall_func_t)0	,R_SHORT	}, /*	38	*/
	{ NAME("procids/setpgrp") (syscall_func_t)0	,R_SHORT	}, /*	39	*/
	{ NAME("cxenix")	(syscall_func_t)0	,R_VOID		}, /*	40	*/
	{ NAME("dup")		emu_i_dup		,R_SHORT	}, /*	41 T	*/
	{ NAME("pipe")		emu_i_pipe		,R_LONG		}, /*	42 T	*/
	{ NAME("times")		emu_i_a			,R_LONG		}, /*	43	*/
	{ NAME("prof")		(syscall_func_t)0	,R_VOID		}, /*	44	*/
	{ NAME("lock/plock")	(syscall_func_t)0	,R_SHORT	}, /*	45	*/
	{ NAME("setgid")	emu_i_s			,R_SHORT	}, /*	46 T	*/
	{ NAME("getgid")	emu_i_s			,R_SHORT	}, /*	47 T	*/
	{ NAME("signal")	emu_i_signal		,R_SPECIAL	}, /*	48 T	*/
	{ NAME("msgsys")	(syscall_func_t)0	,R_SHORT	}, /*	49	*/
	{ NAME("sysi86/sys3b")	(syscall_func_t)0	,R_SHORT	}, /*	50	*/
	{ NAME("acct/sysacct")	(syscall_func_t)0	,R_SHORT	}, /*	51	*/
	{ NAME("shmsys")	(syscall_func_t)0	,R_SHORT	}, /*	52	*/
	{ NAME("semsys")	(syscall_func_t)0	,R_SHORT	}, /*	53	*/
	{ NAME("ioctl")		emu_i_ioctl		,R_SHORT	}, /*	54 T	*/
	{ NAME("uadmin")	(syscall_func_t)0	,R_SHORT	}, /*	55	*/
	{ NAME("?")		(syscall_func_t)0	,R_SHORT	}, /*	56	*/
	{ NAME("utsys")		(syscall_func_t)0	,R_SHORT	}, /*	57	*/
	{ NAME("fsync")		(syscall_func_t)0	,R_SHORT	}, /*	58	*/
	{ NAME("execv")		emu_exec		,R_SHORT	}, /*	59	*/
	{ NAME("umask")		emu_i_s			,R_SHORT	}, /*	60	*/
	{ NAME("chroot")	emu_i_a			,R_SHORT	}, /*	61	*/
	{ NAME("fcntl/clocal")	emu_i_fcntl		,R_SHORT	}, /*	62 T	*/
/*	{ NAME("ulimit/cxenix") emu_i_sls		,R_LONG		}, */ /*	63	*/
    { NAME("ulimit/cxenix") emu_i_ulimit            ,R_LONG         }, /*   63      */
	{ NAME("?")		(syscall_func_t)0	,R_SHORT	}, /*	64	*/
	{ NAME("?")		(syscall_func_t)0	,R_SHORT	}, /*	65	*/
	{ NAME("?")		(syscall_func_t)0	,R_SHORT	}, /*	66	*/
	{ NAME("?")		(syscall_func_t)0	,R_SHORT	}, /*	67	*/
	{ NAME("?")		(syscall_func_t)0	,R_SHORT	}, /*	68	*/
	{ NAME("?")		(syscall_func_t)0	,R_SHORT	}, /*	69	*/
	{ NAME("advfs")		(syscall_func_t)0	,R_SHORT	}, /*	70	*/
	{ NAME("unadvfs")	(syscall_func_t)0	,R_SHORT	}, /*	71	*/
	{ NAME("rmount")	(syscall_func_t)0	,R_SHORT	}, /*	72	*/
	{ NAME("rumount")	(syscall_func_t)0	,R_SHORT	}, /*	73	*/
	{ NAME("rfstart")	(syscall_func_t)0	,R_SHORT	}, /*	74	*/
	{ NAME("?")		(syscall_func_t)0	,R_SHORT	}, /*	75	*/
	{ NAME("rdebug")	(syscall_func_t)0	,R_SHORT	}, /*	76	*/
	{ NAME("rfstop")	(syscall_func_t)0	,R_SHORT	}, /*	77	*/
	{ NAME("rfsys")		(syscall_func_t)0	,R_SHORT	}, /*	78	*/
	{ NAME("rmdir")		(syscall_func_t)0	,R_SHORT	}, /*	79	*/
	{ NAME("mkdir")		(syscall_func_t)0	,R_SHORT	}, /*	80	*/
	{ NAME("getdents")	(syscall_func_t)0	,R_SHORT	}, /*	81	*/
	{ NAME("libattach")	(syscall_func_t)0	,R_SHORT	}, /*	82	*/
	{ NAME("libdetach")	(syscall_func_t)0	,R_SHORT	}, /*	83	*/
	{ NAME("sysfs")		(syscall_func_t)0	,R_SHORT	}, /*	84	*/
	{ NAME("getmsg")	(syscall_func_t)0	,R_SHORT	}, /*	85	*/
	{ NAME("putmsg")	(syscall_func_t)0	,R_SHORT	}, /*	86	*/
	{ NAME("poll")		(syscall_func_t)0	,R_SHORT	}, /*	87	*/
	{ NAME("?")		(syscall_func_t)0	,R_SHORT	}, /*	88	*/
	{ NAME("security")	(syscall_func_t)0	,R_SHORT	}, /*	89	*/
	{ NAME("symlink")	(syscall_func_t)0	,R_SHORT	}, /*	90	*/
	{ NAME("lstat")		(syscall_func_t)0	,R_SHORT	}, /*	91	*/
	{ NAME("readlink")	(syscall_func_t)0	,R_SHORT	}, /*	92	*/
	{ NAME("?")		(syscall_func_t)0	,R_SHORT	}, /*	93	*/
	{ NAME("?")		(syscall_func_t)0	,R_SHORT	}, /*	94	*/
	{ NAME("?")		(syscall_func_t)0	,R_SHORT	}, /*	95	*/
	{ NAME("sigsuspend")	(syscall_func_t)0	,R_SHORT	}, /*	96	*/
	{ NAME("sigaltstack")	(syscall_func_t)0	,R_SHORT	}, /*	97	*/
	{ NAME("sigaction")	(syscall_func_t)0	,R_SHORT	}, /*	98	*/
	{ NAME("sigpending")	(syscall_func_t)0	,R_SHORT	}, /*	99	*/
	{ NAME("context")	(syscall_func_t)0	,R_SHORT	}, /*	100	*/
	{ NAME("evsys")		(syscall_func_t)0	,R_SHORT	}, /*	101	*/
	{ NAME("evtrapret")	(syscall_func_t)0	,R_SHORT	}, /*	102	*/
	{ NAME("statvfs")	(syscall_func_t)0	,R_SHORT	}, /*	103	*/
	{ NAME("fstatvfs")	(syscall_func_t)0	,R_SHORT	}, /*	104	*/
	{ NAME("sysisc")	(syscall_func_t)0	,R_SHORT	}, /*	105	*/
	{ NAME("nfssys")	(syscall_func_t)0	,R_SHORT	}, /*	106	*/
	{ NAME("waitsys")	(syscall_func_t)0	,R_SHORT	}, /*	107	*/
	{ NAME("sigsendsys")	(syscall_func_t)0	,R_SHORT	}, /*	108	*/
	{ NAME("hrtsys")	(syscall_func_t)0	,R_SHORT	}, /*	109	*/
	{ NAME("acancel")	(syscall_func_t)0	,R_SHORT	}, /*	110	*/
	{ NAME("async")		(syscall_func_t)0	,R_SHORT	}, /*	111	*/
	{ NAME("priocntlsys")	(syscall_func_t)0	,R_SHORT	}, /*	112	*/
	{ NAME("pathconf")	(syscall_func_t)0	,R_SHORT	}, /*	113	*/
	{ NAME("mincore")	(syscall_func_t)0	,R_SHORT	}, /*	114	*/
	{ NAME("mmap")		(syscall_func_t)0	,R_SHORT	}, /*	115	*/
	{ NAME("mprotect")	(syscall_func_t)0	,R_SHORT	}, /*	116	*/
	{ NAME("munmap")	(syscall_func_t)0	,R_SHORT	}, /*	117	*/
	{ NAME("fpathconf")	(syscall_func_t)0	,R_SHORT	}, /*	118	*/
	{ NAME("vfork")		(syscall_func_t)0	,R_SHORT	}, /*	119	*/
	{ NAME("fchdir")	(syscall_func_t)0	,R_SHORT	}, /*	120	*/
	{ NAME("readv")		(syscall_func_t)0	,R_SHORT	}, /*	121	*/
	{ NAME("writev")	(syscall_func_t)0	,R_SHORT	}, /*	122	*/
	{ NAME("xstat")		(syscall_func_t)0	,R_SHORT	}, /*	123	*/
	{ NAME("lxstat")	(syscall_func_t)0	,R_SHORT	}, /*	124	*/
	{ NAME("fxstat")	(syscall_func_t)0	,R_SHORT	}, /*	125	*/
	{ NAME("xmknod")	(syscall_func_t)0	,R_SHORT	}, /*	126	*/
	{ NAME("clocal")	(syscall_func_t)0	,R_SHORT	}, /*	127	*/
	{ NAME("setrlimit")	(syscall_func_t)0	,R_SHORT	}, /*	128	*/
	{ NAME("getrlimit")	(syscall_func_t)0	,R_SHORT	}, /*	129	*/
	{ NAME("lchown")	(syscall_func_t)0	,R_SHORT	}, /*	130	*/
	{ NAME("memcntl")	(syscall_func_t)0	,R_SHORT	}, /*	131	*/
	{ NAME("getpmsg")	(syscall_func_t)0	,R_SHORT	}, /*	132	*/
	{ NAME("putpmsg")	(syscall_func_t)0	,R_SHORT	}, /*	133	*/
	{ NAME("rename")	(syscall_func_t)0	,R_SHORT	}, /*	134	*/
	{ NAME("uname")		(syscall_func_t)0	,R_SHORT	}, /*	135	*/
	{ NAME("setegid")	(syscall_func_t)0	,R_SHORT	}, /*	136	*/
	{ NAME("sysconfig")	(syscall_func_t)0	,R_SHORT	}, /*	137	*/
	{ NAME("adjtime")	(syscall_func_t)0	,R_SHORT	}, /*	138	*/
	{ NAME("systeminfo")	(syscall_func_t)0	,R_SHORT	}, /*	139	*/
	{ NAME("?")		(syscall_func_t)0	,R_SHORT	}, /*	140	*/
	{ NAME("seteuid")	(syscall_func_t)0	,R_SHORT	}, /*	141	*/
	{ NAME("?")		(syscall_func_t)0	,R_SHORT	}, /*	142	*/
	{ NAME("?")		(syscall_func_t)0	,R_SHORT	}  /*	143	*/
};

static syscall_t xenix_syscall[] = {
	{ NAME("shutdn")	(syscall_func_t)0	,R_VOID		}, /*  0	*/
/* joerg 	{ NAME("locking")	emu_i_ssl		,R_SHORT	}, */ /*  1	*/
	{ NAME("locking")	emu_locking		,R_SHORT	}, /*  1	*/
	{ NAME("creatsem")	(syscall_func_t)0	,R_SHORT	}, /*  2	*/
	{ NAME("opensem")	(syscall_func_t)0	,R_SHORT	}, /*  3	*/
	{ NAME("sigsem")	(syscall_func_t)0	,R_SHORT	}, /*  4	*/
	{ NAME("waitsem")	(syscall_func_t)0	,R_SHORT	}, /*  5	*/
	{ NAME("nbwaitsem")	(syscall_func_t)0	,R_SHORT	}, /*  6	*/
	{ NAME("rdchk")		emu_i_s			,R_SHORT	}, /*  7	*/
	{ NAME("stkgro")	emu_stkgro		,R_SHORT	}, /*  8 T	*/
	{ NAME("xptrace")	(syscall_func_t)0	,R_SHORT	}, /*  9	*/
	{ NAME("chsize")	emu_i_sls		,R_SHORT	}, /* 10	*/
/* joerg	{ NAME("ftime")		emu_i_ftime		,R_VOID		},*/ /* 11 T	*/
	{ NAME("ftime")		emu_i_ftime		,R_SHORT		}, /* 11 T	*/
	{ NAME("nap")		emu_i_l			,R_LONG		}, /* 12	*/
	{ NAME("sdget")		(syscall_func_t)0	,R_POINTER	}, /* 13	*/
	{ NAME("sdfree")	(syscall_func_t)0	,R_SHORT	}, /* 14	*/
	{ NAME("sdenter")	(syscall_func_t)0	,R_SHORT	}, /* 15	*/
	{ NAME("sdleave")	(syscall_func_t)0	,R_SHORT	}, /* 16	*/
	{ NAME("sdgetv")	(syscall_func_t)0	,R_SHORT	}, /* 17	*/
	{ NAME("sdwaitv")	(syscall_func_t)0	,R_SHORT	}, /* 18	*/
	{ NAME("brkctl")	emu_brkctl		,R_FPOINTER	}, /* 19 T	*/
	{ NAME("?")		(syscall_func_t)0	,R_SHORT	}, /* 20	*/
	{ NAME("?")		(syscall_func_t)0	,R_SHORT	}, /* 21	*/
	{ NAME("msgctl")	(syscall_func_t)0	,R_SHORT	}, /* 22	*/
	{ NAME("msgget")	(syscall_func_t)0	,R_SHORT	}, /* 23	*/
	{ NAME("msgsnd")	(syscall_func_t)0	,R_SHORT	}, /* 24	*/
	{ NAME("msgrcv")	(syscall_func_t)0	,R_SHORT	}, /* 25	*/
	{ NAME("semctl")	(syscall_func_t)0	,R_SHORT	}, /* 26	*/
	{ NAME("semget")	(syscall_func_t)0	,R_SHORT	}, /* 27	*/
	{ NAME("semop")		(syscall_func_t)0	,R_SHORT	}, /* 28	*/
	{ NAME("shmctl")	(syscall_func_t)0	,R_SHORT	}, /* 29	*/
	{ NAME("shmget")	(syscall_func_t)0	,R_SHORT	}, /* 30	*/
	{ NAME("shmat")		(syscall_func_t)0	,R_FPOINTER	}, /* 31	*/
	{ NAME("proctl")	(syscall_func_t)0	,R_SHORT	}, /* 32	*/
	{ NAME("execseg")	(syscall_func_t)0	,R_FPOINTER	}, /* 33	*/
	{ NAME("unexecseg")	(syscall_func_t)0	,R_SHORT	}, /* 34	*/
	{ NAME("?")		(syscall_func_t)0	,R_SHORT	}, /* 35	*/
	{ NAME("select")	(syscall_func_t)0	,R_SHORT	}, /* 36	*/
	{ NAME("eaccess")	emu_i_ass		,R_SHORT	}, /* 37	*/
	{ NAME("paccess")	(syscall_func_t)0	,R_SHORT	}, /* 38	*/
	{ NAME("sigaction")	(syscall_func_t)0	,R_SHORT	}, /* 39	*/
	{ NAME("sigprocmask")	(syscall_func_t)0	,R_SHORT	}, /* 40	*/
	{ NAME("sigpending")	(syscall_func_t)0	,R_SHORT	}, /* 41	*/
	{ NAME("sigsuspend")	(syscall_func_t)0	,R_SHORT	}, /* 42	*/
	{ NAME("getgroups")	(syscall_func_t)0	,R_SHORT	}, /* 43	*/
	{ NAME("setgroups")	(syscall_func_t)0	,R_SHORT	}, /* 44	*/
	{ NAME("sysconf")	(syscall_func_t)0	,R_SHORT	}, /* 45	*/
	{ NAME("pathconf")	(syscall_func_t)0	,R_SHORT	}, /* 46	*/
	{ NAME("fpathconf")	(syscall_func_t)0	,R_SHORT	}, /* 47	*/
	{ NAME("rename")	emu_i_aa		,R_SHORT	}, /* 48	*/
	{ NAME("?")		(syscall_func_t)0	,R_SHORT	}, /* 49	*/
	{ NAME("utsname")	emu_i_a			,R_SHORT	}, /* 50	*/
	{ NAME("?")		(syscall_func_t)0	,R_SHORT	}, /* 51	*/
	{ NAME("?")		(syscall_func_t)0	,R_SHORT	}, /* 52	*/
	{ NAME("?")		(syscall_func_t)0	,R_SHORT	}, /* 53	*/
	{ NAME("?")		(syscall_func_t)0	,R_SHORT	}, /* 54	*/
	{ NAME("gettimer")	(syscall_func_t)0	,R_SHORT	}, /* 55	*/
	{ NAME("setitimer")	(syscall_func_t)0	,R_SHORT	}, /* 56	*/
	{ NAME("?")		(syscall_func_t)0	,R_SHORT	}, /* 57	*/
	{ NAME("?")		(syscall_func_t)0	,R_SHORT	}, /* 58	*/
	{ NAME("?")		(syscall_func_t)0	,R_SHORT	}, /* 59	*/
	{ NAME("?")		(syscall_func_t)0	,R_SHORT	}, /* 60	*/
	{ NAME("?")		(syscall_func_t)0	,R_SHORT	}, /* 61	*/
	{ NAME("?")		(syscall_func_t)0	,R_SHORT	}, /* 62	*/
	{ NAME("?")		(syscall_func_t)0	,R_SHORT	}  /* 63	*/
};


/* Entry:
 *
 * For small and med model:
 * The syscall number is in ax and the arguments are in bx, cx, si, di
 * (in that order reading the C function call left to right).
 *
 * For Large model:
 * The syscall number is in ax and bx points to the location of the
 * arguments on the stack.
 *
 * Returns:
 *	int or char types in ax
 *	long in ax and bx with high word in bx
 *	long addresses in ax and bx with the segment selector in bx
 *	struct and floats are placed in static memory and a pointer returned
 *
 * Note: I think documentation for 286 function entry/exit says dx rather
 * than bx. However after a syscall the first thing that happens is that
 * bx is moved into dx. Syscalls do it one way, functions another. Love
 * the consistency :-).
 */
void
x286syscall(struct sigcontext *sc)
{
	int error,ret_type;
	unsigned int call_no;
	unsigned short *stkladdr;
	unsigned int opcode = (sc->eax & 0xffff);

	error = -1;
	xerrno = ENOSYS;
	ret_type = R_SHORT;
	if ((sc->eax & 0xff) == 0x28) {
		call_no = (sc->eax >> 8) & 0xff;
		if (call_no < sizeof(xenix_syscall)/sizeof(xenix_syscall[0])) {
			if(LDATA){	/* Large Data */
				stkladdr = (unsigned short *)(ldt[sc->ss >> 3].base + (sc->ebx & 0xffff));
				d_print("x286emul: %s (0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x)\n",
					xenix_syscall[call_no].name,
					stkladdr[0], stkladdr[1],
					stkladdr[2], stkladdr[3],
					stkladdr[4]);
			}
			else {
				d_print("x286emul: %s (0x%04lx, 0x%04lx, 0x%04lx, 0x%04lx)\n",
					xenix_syscall[call_no].name,
					sc->ebx & 0xffff, sc->ecx & 0xffff,
					sc->esi & 0xffff, sc->edi & 0xffff);
			}
			if (xenix_syscall[call_no].func)
				error = xenix_syscall[call_no].func(sc);
			ret_type = xenix_syscall[call_no].ret_type;
		}
	} else {
		call_no = sc->eax & 0xff;
		if (call_no < sizeof(unix_syscall)/sizeof(unix_syscall[0])) {
			if(LDATA){	/* Large Data */
				stkladdr = (unsigned short *)(ldt[sc->ss >> 3].base + (sc->ebx & 0xffff));
				d_print("x286emul: %s (0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x)\n",
					unix_syscall[call_no].name,
					stkladdr[0], stkladdr[1],
					stkladdr[2], stkladdr[3],
					stkladdr[4]);
			}
			else {
				d_print("x286emul: %s (0x%04lx, 0x%04lx, 0x%04lx, 0x%04lx)\n",
					unix_syscall[call_no].name,
					sc->ebx & 0xffff, sc->ecx & 0xffff,
					sc->esi & 0xffff, sc->edi & 0xffff);
			}
			if (unix_syscall[call_no].func)
				error = unix_syscall[call_no].func(sc);
			ret_type = unix_syscall[call_no].ret_type;
		}
	}
	
	if (ret_type == R_VOID || ret_type == R_SPECIAL){
		if(ret_type == R_VOID)
			error = 0;
	} else {
		if (error < 0) {
			sc->eax = xerrno;
			if(ret_type == R_LONG)
				sc->ebx = 0;
			if((ret_type == R_POINTER && LDATA) || (ret_type = R_FPOINTER))
				sc->ebx = 0xff;
			sc->eflags |= 1;
		} else {
			sc->eax = error & 0xffff;
			if((ret_type == R_LONG) || 
				(ret_type == R_POINTER && LDATA) ||
				(ret_type == R_FPOINTER))
					sc->ebx = (error >> 16) & 0xffff;
			sc->eflags &= (~1);
		}
	}

	if (error < 0) {
		d_print("x286emul:   syscall 0x%04x gave error %d\n",
			opcode, xerrno);
	} else {
		d_print("x286emul:   syscall 0x%04x returned 0x%04lx:0x%04lx\n",
			opcode, sc->ebx, sc->eax);
	}
}


extern sigset_t signo_pending;

void
_x286syscall(struct sigcontext sc)
{
	unsigned char *laddr;
	unsigned short *stkladdr;
	int signo;

#ifdef DEBUG_STACK
	unsigned int s,p;
	__asm__ volatile ("\tmovl $0,%%eax\n"
		"\tmov %%ss,%%ax\n"
		"\tmovl %%esp,%%ebx\n"
		: "=a" (s),"=b" (p)
		: );
	d_print("\nx286call: stack now -> 0x%02x:0x%04x (0x%08lx)!\n",
			s,
			p,
			ldt[s >> 3].base + ((ldt[s >> 3].base)?(p & 0xffff):(p)) );
#endif

	/* Look up the linear address for the segment:offset of the
	 * eip and stack.
	 */
	stkladdr = (unsigned short *)(ldt[sc.ss >> 3].base
			+ (sc.esp_at_signal & 0xffff));
	stkladdr[-1] = stkladdr[-2];
	stkladdr[-2] = stkladdr[-4];
	sc.eip = stkladdr[-2];		/* get return address off stack */
	sc.cs = stkladdr[-1];

	laddr = (unsigned char *)(ldt[sc.cs >> 3].base + sc.eip);
	dump_state(laddr, stkladdr, &sc);

	x286syscall(&sc);

	for (signo = 1 ; signo < NSIG ; signo++) {	/* action any signals that were delayed */
		if( sigismember(&signo_pending, signo)) {
			d_print("x286emul:   after syscall: sig is pending 0x%04x\n", signo);
			sigdelset(&signo_pending, signo);
			do_sig_pending(signo, &sc);
			d_print("x286emul:   after syscall: done do_sig_pending for 0x%04x\n", signo);
		}
	}

	/*
	 * Put flags on the stack just in case we were
	 * interrupted.
	 */
	stkladdr[-1] = (unsigned short)(sc.eflags & 0xffff);

	stkladdr = (unsigned short *)(ldt[sc.ss >> 3].base
			+ (sc.esp & 0xffff));
	stkladdr[-2] = (unsigned short)sc.eip;	/* put return address back on stack */
	stkladdr[-1] = (unsigned short)sc.cs;

}
