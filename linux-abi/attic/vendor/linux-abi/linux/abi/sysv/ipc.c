/*
 *  linux/abi/svr4_common/ipc.c
 *
 *  (C) Copyright 1994  Mike Jagdis (jaggy@purplet.demon.co.uk)
 *
 *  Massive work over with a fine tooth comb, lots of rewriting. There
 *  were a *lot* of bugs in this - mismatched structs that weren't
 *  mapped, wrong pointers etc. I've tested this version with the
 *  demo programs from the Wyse V/386 IPC documentation which exercise
 *  all the functions. I don't have any major IPC using applications
 *  to test it with - as far as I know...
 *
 *  Original copyright etc. follows:
 *
 *  Copyright (C) 1993,1994  Joe Portman (baron@hebron.connected.com)
 *	 First stab at ibcs shm, sem and msg handlers
 *
 *  NOTE:
 *  Please contact the author above before blindly making changes
 *  to this file. You will break things.
 *
 *  04-15-1994 JLP III
 *  Still no msgsys, but IPC_STAT now works for shm calls
 *  Corrected argument order for sys_ipc calls, to accomodate Mike's
 *  changes, so that we can just call sys_ipc instead of the internal
 *  sys_* calls for ipc functions.
 *  Cleaned up translation of perm structures
 *  tstshm for Oracle now works.
 *
 *  04-23-1994 JLP III
 *  Added in msgsys calls, Tested and working
 *  Added translation for IPC_SET portions of all xxxctl functions.
 *  Added SHM_LOCK and SHM_UNLOCK to shmsys
 *
 *  04-28-1994 JLP III
 *  Special thanks to Brad Pepers for adding in the GETALL and SETALL
 *  case of semaphores. (pepersb@cuug.ab.ca)
 *
 * $Id$
 * $Source$
 */
#include <linux/config.h>
#define __NO_VERSION__
#include <linux/module.h>
#include <linux/version.h>

#include <asm/uaccess.h>

#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/stddef.h>
#include <linux/unistd.h>
#include <linux/ptrace.h>

#include <asm/system.h>
#include <linux/fs.h>
#include <linux/sys.h>
#include <asm/ipc.h>
#include <linux/ipc.h>
#include <linux/sem.h>
#include <linux/shm.h>
#include <linux/msg.h>
#include <linux/string.h>

#include <abi/abi.h>
#include <abi/trace.h>


struct ibcs_ipc_perm {
	unsigned short uid;		/* owner's user id */
	unsigned short gid;		/* owner's group id */
	unsigned short cuid;		/* creator's user id */
	unsigned short cgid;		/* creator's group id */
	unsigned short mode;		/* access modes */
	unsigned short seq;		/* slot usage sequence number */
	long key;			/* key */
};

struct ibcs_ipc_perm_l {
	unsigned long uid;		/* owner's user id */
	unsigned long gid;		/* owner's group id */
	unsigned long cuid;		/* creator's user id */
	unsigned long cgid;		/* creator's group id */
	unsigned long mode;		/* access modes */
	unsigned long seq;		/* slot usage sequence number */
	long key;			/* key */
	void *ipc_secp;			/* security structure pointer */
	long pad[3];			/* reserved */
};

struct ibcs_semid_ds {
	struct ibcs_ipc_perm sem_perm;
	struct sem *sem_base;
	unsigned short sem_nsems;
	char __pad[2];
	unsigned long sem_otime;
	unsigned long sem_ctime;
};

struct ibcs_semid_ds_l {
	struct ibcs_ipc_perm_l sem_perm;
	struct sem *sem_base;
	unsigned short sem_nsems;
	char __pad[2];
	unsigned long sem_otime;
	unsigned long sem_pad1;
	unsigned long sem_ctime;
	unsigned long sem_pad2;
	unsigned long sem_pad3[4];
};

struct ibcs_shmid_ds {
	struct ibcs_ipc_perm shm_perm;	/* operation permission struct */
	int shm_segsz;			/* size of segment in bytes */
	struct region *__pad1;		/* ptr to region structure */
	char __pad2[4];			/* for swap compatibility */
	ushort shm_lpid;		/* pid of last shmop */
	ushort shm_cpid;		/* pid of creator */
	unsigned short shm_nattch;	/* used only for shminfo */
	unsigned short __pad3;
	time_t shm_atime;		/* last shmat time */
	time_t shm_dtime;		/* last shmdt time */
	time_t shm_ctime;		/* last change time */
};

struct ibcs_shmid_ds_l {
	struct ibcs_ipc_perm_l shm_perm;/* operation permission struct */
	int shm_segsz;			/* size of segment in bytes */
	struct region *__pad1;		/* ptr to region structure */
	unsigned short shm_lckcnt;	/* number of times it is being locked */
	char __pad2[2];			/* for swap compatibility */
	unsigned long shm_lpid;		/* pid of last shmop */
	unsigned long shm_cpid;		/* pid of creator */
	unsigned long shm_nattch;	/* used only for shminfo */
	unsigned long shm_cnattch;
	unsigned long shm_atime;		/* last shmat time */
	unsigned long shm_pad1;
	unsigned long shm_dtime;		/* last shmdt time */
	unsigned long shm_pad2;
	unsigned long shm_ctime;		/* last change time */
	unsigned long shm_pad3;
	unsigned long shm_pad4[4];
};

struct ibcs_msqid_ds {
	struct ibcs_ipc_perm msg_perm;
	struct msg *msg_first;
	struct msg *msg_last;
	ushort msg_cbytes;
	ushort msg_qnum;
	ushort msg_qbytes;
	ushort msg_lspid;
	ushort msg_lrpid;
	time_t msg_stime;
	time_t msg_rtime;
	time_t msg_ctime;
};

struct ibcs_msqid_ds_l {
	struct ibcs_ipc_perm_l msg_perm;
	struct msg *msg_first;
	struct msg *msg_last;
	unsigned long msg_cbytes;
	unsigned long msg_qnum;
	unsigned long msg_qbytes;
	unsigned long msg_lspid;
	unsigned long msg_lrpid;
	unsigned long msg_stime;
	unsigned long msg_pad1;
	unsigned long msg_rtime;
	unsigned long msg_pad2;
	unsigned long msg_ctime;
	unsigned long msg_pad3;
	unsigned long msg_pad4[4];
};


static inline void ip_to_lp(struct ibcs_ipc_perm *ip, struct ipc_perm *lp)
{
	lp->uid = ip->uid;
	lp->gid = ip->gid;
	lp->cuid = ip->cuid;
	lp->cgid = ip->cgid;
	lp->mode = ip->mode;
	lp->seq = ip->seq;
	lp->key = ip->key;
}

static inline void lp_to_ip(struct ipc_perm *lp, struct ibcs_ipc_perm *ip)
{
	ip->uid = lp->uid;
	ip->gid = lp->gid;
	ip->cuid = lp->cuid;
	ip->cgid = lp->cgid;
	ip->mode = lp->mode;
	ip->seq = lp->seq;
	ip->key = lp->key;
}

static inline void ip_to_lp_l(struct ibcs_ipc_perm_l *ip, struct ipc_perm *lp)
{
	lp->uid = ip->uid;
	lp->gid = ip->gid;
	lp->cuid = ip->cuid;
	lp->cgid = ip->cgid;
	lp->mode = ip->mode;
	lp->seq = ip->seq;
	lp->key = ip->key;
}

static inline void lp_to_ip_l(struct ipc_perm *lp, struct ibcs_ipc_perm_l *ip)
{
	ip->uid = lp->uid;
	ip->gid = lp->gid;
	ip->cuid = lp->cuid;
	ip->cgid = lp->cgid;
	ip->mode = lp->mode;
	ip->seq = lp->seq;
	ip->key = lp->key;
}


#define U_SEMCTL    (0)
#define U_SEMGET    (1)
#define U_SEMOP     (2)
#define U_SHMLOCK   (3)
#define U_SHMUNLOCK (4)

#define U_IPC_RMID	0
#define U_IPC_SET	1
#define U_IPC_STAT	2
#define U_GETNCNT	3
#define U_GETPID	4
#define U_GETVAL	5
#define U_GETALL	6
#define U_GETZCNT	7
#define U_SETVAL	8
#define U_SETALL	9
#define U_IPC_RMID_L	10
#define U_IPC_SET_L	11
#define U_IPC_STAT_L	12

static inline int ibcs_sem_trans(int arg)
{
	switch (arg) {
		case U_IPC_RMID:	return IPC_RMID;
		case U_IPC_SET:		return IPC_SET;
		case U_IPC_STAT:	return IPC_STAT;
		case U_GETNCNT:		return GETNCNT;
		case U_GETPID:		return GETPID;
		case U_GETVAL:		return GETVAL;
		case U_GETALL:		return GETALL;
		case U_GETZCNT:		return GETZCNT;
		case U_SETVAL:		return SETVAL;
		case U_SETALL:		return SETALL;
		case U_IPC_RMID_L:	return IPC_RMID;
		case U_IPC_SET_L:	return U_IPC_SET_L;
		case U_IPC_STAT_L:	return U_IPC_STAT_L;
	}
	return -1;
}

static void isem_to_lsem(struct ibcs_semid_ds *is, struct semid_ds *ls)
{
	ip_to_lp(&is->sem_perm, &ls->sem_perm);
	ls->sem_base = is->sem_base;
	ls->sem_nsems = is->sem_nsems;
	ls->sem_otime = is->sem_otime;
	ls->sem_ctime = is->sem_ctime;
}

static void lsem_to_isem(struct semid_ds *ls, struct ibcs_semid_ds *is)
{
	lp_to_ip(&ls->sem_perm, &is->sem_perm);
	is->sem_base = ls->sem_base;
	is->sem_nsems = ls->sem_nsems;
	is->sem_otime = ls->sem_otime;
	is->sem_ctime = ls->sem_ctime;
}

static void isem_to_lsem_l(struct ibcs_semid_ds_l *is, struct semid_ds *ls)
{
	ip_to_lp_l(&is->sem_perm, &ls->sem_perm);
	ls->sem_base = is->sem_base;
	ls->sem_nsems = is->sem_nsems;
	ls->sem_otime = is->sem_otime;
	ls->sem_ctime = is->sem_ctime;
}

static void lsem_to_isem_l(struct semid_ds *ls, struct ibcs_semid_ds_l *is)
{
	memset(is, 0, sizeof(*is));
	lp_to_ip_l(&ls->sem_perm, &is->sem_perm);
	is->sem_base = ls->sem_base;
	is->sem_nsems = ls->sem_nsems;
	is->sem_otime = ls->sem_otime;
	is->sem_ctime = ls->sem_ctime;
}

int svr4_semsys(struct pt_regs *regs)
{
	int command = get_syscall_parameter (regs, 0);
	int arg1, arg2, arg3;
	union semun *arg4;
	struct semid_ds ls;
	union semun lsemun;
	mm_segment_t old_fs;
	int retval;

	arg1 = get_syscall_parameter (regs, 1);
	arg2 = get_syscall_parameter (regs, 2);
	arg3 = get_syscall_parameter (regs, 3);
	switch (command) {
		case U_SEMCTL:
			/* XXX - The value for arg4 depends on how union
			 * passing is implement on this architecture and
			 * compiler. The following is *only* known to be
			 * right for Intel (the default else case).
			 */
#ifdef __sparc__
			arg4 = (union semun *)get_syscall_parameter (regs, 4);
#else
			arg4 = (union semun *)(((unsigned long *) regs->esp) + (5));
#endif
#ifdef CONFIG_ABI_TRACE
			if ((ibcs_trace & TRACE_API) || ibcs_func_p->trace)
			printk(KERN_DEBUG "%d iBCS: ibcs_semctl: args: %d %d %d %lx\n",
					current->pid,
					 arg1, arg2, arg3, (unsigned long)arg4);
#endif
			switch (arg3) {
				case U_IPC_SET: {
					struct ibcs_semid_ds is, *is_p;

					retval = get_user(is_p, (struct ibcs_semid_ds **)&arg4->buf);
					if (!retval)
						retval = verify_area(VERIFY_WRITE, is_p, sizeof(is));
					if (retval)
						return retval;

					copy_from_user(&is, (char *)is_p, sizeof(is));
					isem_to_lsem(&is, &ls);

					lsemun.buf = &ls;
					old_fs = get_fs();
					set_fs (get_ds());
					retval = SYS (ipc) (SEMCTL, arg1, arg2, IPC_SET, &lsemun);
					set_fs(old_fs);

					lsem_to_isem(&ls, &is);
					copy_to_user((char *)is_p, &is, sizeof(is));
					return retval;
				}

				case U_IPC_SET_L: {
					struct ibcs_semid_ds_l is, *is_p;

					retval = get_user(is_p, (struct ibcs_semid_ds_l **)&arg4->buf);
					if (!retval)
						retval = verify_area(VERIFY_WRITE, is_p, sizeof(is));
					if (retval)
						return retval;

					copy_from_user(&is, (char *)is_p, sizeof(is));
					isem_to_lsem_l(&is, &ls);

					lsemun.buf = &ls;
					old_fs = get_fs();
					set_fs (get_ds());
					retval = SYS (ipc) (SEMCTL, arg1, arg2, IPC_SET, &lsemun);
					set_fs(old_fs);

					lsem_to_isem_l(&ls, &is);
					copy_to_user((char *)is_p, &is, sizeof(is));
					return retval;
				}

				case U_IPC_RMID:
				case U_IPC_RMID_L:
				case U_SETVAL:
				case U_GETVAL:
				case U_GETPID:
				case U_GETNCNT:
				case U_GETZCNT: {
					int cmd = ibcs_sem_trans(arg3);
					return SYS (ipc) (SEMCTL, arg1, arg2, cmd, arg4);
				}

				case U_SETALL:
				case U_GETALL: {
					int cmd = ibcs_sem_trans(arg3);
					return SYS (ipc) (SEMCTL, arg1, 0, cmd, arg4);
				}

				case U_IPC_STAT: {
					struct ibcs_semid_ds is, *is_p;

					retval = get_user(is_p, (struct ibcs_semid_ds **)&arg4->buf);
					if (!retval)
						retval = verify_area(VERIFY_WRITE, (char *)is_p, sizeof(is));
					if (retval)
						return retval;

					lsemun.buf = &ls;
					old_fs = get_fs();
					set_fs(get_ds());
					retval = SYS (ipc) (SEMCTL, arg1, 0, IPC_STAT, &lsemun);
					set_fs(old_fs);
					if (retval < 0)
						return retval;

					lsem_to_isem(&ls, &is);
					copy_to_user((char *)is_p, &is, sizeof(is));
					return retval;
				}

				case U_IPC_STAT_L: {
					struct ibcs_semid_ds_l is, *is_p;

					retval = get_user(is_p, (struct ibcs_semid_ds_l **)&arg4->buf);
					if (!retval)
						retval = verify_area(VERIFY_WRITE, (char *)is_p, sizeof(is));
					if (retval)
						return retval;

					lsemun.buf = &ls;
					old_fs = get_fs();
					set_fs(get_ds());
					retval = SYS (ipc) (SEMCTL, arg1, 0, IPC_STAT, &lsemun);
					set_fs(old_fs);
					if (retval < 0)
						return retval;

					lsem_to_isem_l(&ls, &is);
					copy_to_user((char *)is_p, &is, sizeof(is));
					return retval;
				}

				default:
					printk(KERN_ERR "%d ibcs_semctl: unsupported command %d\n",
						current->pid, arg3);
					return -EINVAL;
		  	}

		case U_SEMGET:
#ifdef CONFIG_ABI_TRACE
			if ((ibcs_trace & TRACE_API) || ibcs_func_p->trace)
				printk(KERN_DEBUG "%d iBCS: ibcs_semget: args: %d %d %o \n",
					current->pid,
					arg1, arg2, arg3);
#endif
			return SYS (ipc) (SEMGET, arg1, arg2, arg3, 0);

		case U_SEMOP:
#ifdef CONFIG_ABI_TRACE
			if ((ibcs_trace & TRACE_API) || ibcs_func_p->trace) {
				int x;
				struct sembuf tmp;
				struct sembuf *tp = (struct sembuf *) arg2;

				printk(KERN_DEBUG "%d iBCS: ibcs_semop: args: %d 0x%08lx %d\n",
					current->pid,
					arg1, (unsigned long)arg2, arg3);
				for (x = 0; x < arg3; x++) {
					copy_from_user (&tmp, tp, sizeof (tmp));
					printk(KERN_DEBUG "%d iBCS: ibcs_semop args: %d %d 0%o \n",
						current->pid,
						tmp.sem_num, tmp.sem_op, tmp.sem_flg);
					tp++;
				}
			}
#endif
			return SYS (ipc) (SEMOP, arg1, arg3, 0, (struct sembuf *) arg2);
	}
	return -EINVAL;
}

EXPORT_SYMBOL(svr4_semsys);


#define U_SHMAT  (0)
#define U_SHMCTL (1)
#define U_SHMDT  (2)
#define U_SHMGET (3)

static void
ishm_to_lshm(struct ibcs_shmid_ds *is, struct shmid_ds *ls)
{
	ip_to_lp(&is->shm_perm, &ls->shm_perm);
	ls->shm_segsz = is->shm_segsz;
	ls->shm_lpid = is->shm_lpid;
	ls->shm_cpid = is->shm_cpid;
	ls->shm_nattch = is->shm_nattch;
	ls->shm_atime = is->shm_atime;
	ls->shm_dtime = is->shm_dtime;
	ls->shm_ctime = is->shm_ctime;
}

static void
lshm_to_ishm(struct shmid_ds *ls, struct ibcs_shmid_ds *is)
{
	lp_to_ip(&ls->shm_perm, &is->shm_perm);
	is->shm_segsz = ls->shm_segsz;
	is->shm_lpid = ls->shm_lpid;
	is->shm_cpid = ls->shm_cpid;
	is->shm_nattch = ls->shm_nattch;
	is->shm_atime = ls->shm_atime;
	is->shm_dtime = ls->shm_dtime;
	is->shm_ctime = ls->shm_ctime;
}

static void
ishm_to_lshm_l(struct ibcs_shmid_ds_l *is, struct shmid_ds *ls)
{
	ip_to_lp_l(&is->shm_perm, &ls->shm_perm);
	ls->shm_segsz = is->shm_segsz;
	ls->shm_lpid = is->shm_lpid;
	ls->shm_cpid = is->shm_cpid;
	ls->shm_nattch = is->shm_nattch;
	ls->shm_atime = is->shm_atime;
	ls->shm_dtime = is->shm_dtime;
	ls->shm_ctime = is->shm_ctime;
}

static void lshm_to_ishm_l(struct shmid_ds * ls, struct ibcs_shmid_ds_l * is)
{
	memset(is, 0, sizeof(*is));
	lp_to_ip_l(&ls->shm_perm, &is->shm_perm);
	is->shm_segsz = ls->shm_segsz;
	is->shm_lpid = ls->shm_lpid;
	is->shm_cpid = ls->shm_cpid;
	is->shm_nattch = ls->shm_nattch;
	is->shm_atime = ls->shm_atime;
	is->shm_dtime = ls->shm_dtime;
	is->shm_ctime = ls->shm_ctime;
}


int svr4_shmsys(struct pt_regs * regs)
{
	int command = get_syscall_parameter (regs, 0);
	int arg1, arg2, arg3;
	mm_segment_t old_fs;
	long retval = 0;
	char *addr = 0;

	arg1 = arg2 = arg3 = 0;
	switch (command) {
		case U_SHMAT:
		case U_SHMCTL:
		case U_SHMGET:
			arg1 = get_syscall_parameter (regs, 1);
			arg2 = get_syscall_parameter (regs, 2);
			arg3 = get_syscall_parameter (regs, 3);
			break;
		case U_SHMDT:
			addr = (char *) get_syscall_parameter (regs, 1);
			break;
		default:
			printk(KERN_ERR "%d iBCS: bad SHM command %d\n",
				current->pid, command);
			retval = -EINVAL;
			goto test_exit;
	}

	switch (command) {
		case U_SHMAT: {
#ifdef IPCCALL
			unsigned long raddr;
#endif
#ifdef CONFIG_ABI_TRACE
			if ((ibcs_trace & TRACE_API) || ibcs_func_p->trace)
				printk(KERN_DEBUG "%d iBCS: ibcs_shmat: args: %d %x %o \n",
					current->pid,
					arg1, arg2, arg3);
#endif
			/*
			 * raddr = 0 tells sys_shmat to limit to 2G
			 *	and we are IBCS, no raddr value to return
			 */
#ifdef IPCCALL
			old_fs = get_fs();
			set_fs(get_ds());
			retval = SYS (ipc) (IPCCALL(1,SHMAT), arg1, arg3, &raddr, (char *) arg2);
			set_fs(old_fs);
			if (retval >= 0)
				retval = raddr;
#else
			retval = SYS (ipc) (SHMAT, arg1, arg3, 0, (char *) arg2);
#endif

#ifdef CONFIG_ABI_TRACE
			if ((ibcs_trace & TRACE_API) || ibcs_func_p->trace)
				printk(KERN_DEBUG "%d iBCS: ibcs_shmat: return val is %lx\n",
					current->pid, retval);
#endif
			goto test_exit;
		}

		case U_SHMGET:
#ifdef CONFIG_ABI_TRACE
			if ((ibcs_trace & TRACE_API) || ibcs_func_p->trace)
				printk(KERN_DEBUG "%d iBCS: ibcs_shmget: args: %d %x %o \n",
					current->pid,
					arg1, arg2, arg3);
#endif
			retval = SYS (ipc) (SHMGET, arg1, arg2, arg3, 0);
			goto test_exit;

		case U_SHMDT:
#ifdef CONFIG_ABI_TRACE
			if ((ibcs_trace & TRACE_API) || ibcs_func_p->trace)
				printk(KERN_DEBUG "%d iBCS: ibcs_shmdt: arg: %lx\n",
					current->pid, (unsigned long)addr);
#endif
			retval = SYS (ipc) (SHMDT, 0, 0, 0, addr);
			goto test_exit;

		case U_SHMCTL:
#ifdef CONFIG_ABI_TRACE
			if ((ibcs_trace & TRACE_API) || ibcs_func_p->trace)
				printk(KERN_DEBUG "%d iBCS: ibcs_shmctl: args: %d %x %o %d %x\n",
					current->pid,
					arg1, arg2, arg3, arg3, arg3);
#endif
			switch (arg2) {
				case U_SHMLOCK:
					retval = SYS (ipc) (SHMCTL, arg1, SHM_LOCK, 0, arg3);
					goto test_exit;

				case U_SHMUNLOCK:
					retval = SYS (ipc) (SHMCTL, arg1, SHM_UNLOCK, 0, arg3);
					goto test_exit;

				case U_IPC_SET: {
					struct ibcs_shmid_ds is;
					struct shmid_ds ls;

					retval = verify_area(VERIFY_WRITE, (char *)arg3, sizeof(is));
					if (retval)
						goto test_exit;

					copy_from_user(&is, (char *)arg3, sizeof(is));
					ishm_to_lshm(&is, &ls);

					old_fs = get_fs();
					set_fs (get_ds());
					retval = SYS (ipc) (SHMCTL, arg1, IPC_SET, 0, &ls);
					set_fs(old_fs);
					if (retval < 0)
						goto test_exit;

					lshm_to_ishm(&ls, &is);
					copy_to_user((char *)arg3, &is, sizeof(is));
					goto test_exit;
				}

				case U_IPC_SET_L: {
					struct ibcs_shmid_ds_l is;
					struct shmid_ds ls;

					retval = verify_area(VERIFY_WRITE, (char *)arg3, sizeof(is));
					if (retval)
						goto test_exit;

					copy_from_user(&is, (char *)arg3, sizeof(is));
					ishm_to_lshm_l(&is, &ls);

					old_fs = get_fs();
					set_fs (get_ds());
					retval = SYS (ipc) (SHMCTL, arg1, IPC_SET, 0, &ls);
					set_fs(old_fs);
					if (retval < 0)
						goto test_exit;

					lshm_to_ishm_l(&ls, &is);
					copy_to_user((char *)arg3, &is, sizeof(is));
					goto test_exit;
				}

				case U_IPC_RMID:
				case U_IPC_RMID_L:
					retval = SYS (ipc) (SHMCTL, arg1, IPC_RMID, arg3);
					goto test_exit;

				case U_IPC_STAT: {
					struct ibcs_shmid_ds is;
					struct shmid_ds ls;

					old_fs = get_fs();
					set_fs (get_ds());
					retval = SYS (ipc) (SHMCTL, arg1, IPC_STAT, 0, &ls);
					set_fs(old_fs);
					if (retval < 0)
						goto test_exit;

					lshm_to_ishm(&ls, &is);
					retval = copy_to_user((char *)arg3, &is, sizeof(is)) ? -EFAULT : 0;
					goto test_exit;
				}

				case U_IPC_STAT_L: {
					struct ibcs_shmid_ds_l is;
					struct shmid_ds ls;

					old_fs = get_fs();
					set_fs (get_ds());
					retval = SYS (ipc) (SHMCTL, arg1, IPC_STAT, 0, &ls);
					set_fs(old_fs);
					if (retval < 0)
						goto test_exit;

					lshm_to_ishm_l(&ls, &is);
					retval = copy_to_user((char *)arg3, &is, sizeof(is)) ? -EFAULT : 0;
					goto test_exit;
				}

				default:
					printk(KERN_ERR "%d iBCS: ibcs_shmctl: unsupported command %d\n",
						current->pid, arg2);
			}
			retval = -EINVAL;
			goto test_exit;

		default:
#ifdef CONFIG_ABI_TRACE
			printk(KERN_DEBUG "%d iBCS: ibcs_shm: command: %x\n",
				current->pid, command);
#endif
			retval = -EINVAL;
			goto test_exit;
	}

test_exit:;
	if ((retval < 0) && (retval > -255)) {
	        set_error (regs, iABI_errors (-retval));
#ifdef CONFIG_ABI_TRACE
		if ((ibcs_trace & TRACE_API) || ibcs_func_p->trace)
			printk(KERN_DEBUG "%d iBCS: Error %ld\n",
				current->pid, get_result (regs));
#endif
	} else {
	    	clear_error (regs);
		set_result (regs, retval);
	}

	return 0;
}

EXPORT_SYMBOL(svr4_shmsys);


#define U_MSGGET  (0)
#define U_MSGCTL  (1)
#define U_MSGRCV  (2)
#define U_MSGSND  (3)

static void imsq_to_lmsq(struct ibcs_msqid_ds * im, struct msqid_ds * lm)
{
	ip_to_lp(&im->msg_perm, &lm->msg_perm);
	lm->msg_first = im->msg_first;
	lm->msg_last = im->msg_last;
	lm->msg_cbytes = im->msg_cbytes;
	lm->msg_qnum = im->msg_qnum;
	lm->msg_qbytes = im->msg_qbytes;
	lm->msg_lspid = im->msg_lspid;
	lm->msg_lrpid = im->msg_lrpid;
	lm->msg_stime = im->msg_stime;
	lm->msg_rtime = im->msg_rtime;
	lm->msg_ctime = im->msg_ctime;
}

static void
lmsq_to_imsq(struct msqid_ds *lm, struct ibcs_msqid_ds *im)
{
	lp_to_ip(&lm->msg_perm, &im->msg_perm);
	im->msg_first = lm->msg_first;
	im->msg_last = lm->msg_last;
	im->msg_cbytes = lm->msg_cbytes;
	im->msg_qnum = lm->msg_qnum;
	im->msg_qbytes = lm->msg_qbytes;
	im->msg_lspid = lm->msg_lspid;
	im->msg_lrpid = lm->msg_lrpid;
	im->msg_stime = lm->msg_stime;
	im->msg_rtime = lm->msg_rtime;
	im->msg_ctime = lm->msg_ctime;
}

static void
imsq_to_lmsq_l(struct ibcs_msqid_ds_l *im, struct msqid_ds *lm)
{
	ip_to_lp_l(&im->msg_perm, &lm->msg_perm);
	lm->msg_first = im->msg_first;
	lm->msg_last = im->msg_last;
	lm->msg_cbytes = im->msg_cbytes;
	lm->msg_qnum = im->msg_qnum;
	lm->msg_qbytes = im->msg_qbytes;
	lm->msg_lspid = im->msg_lspid;
	lm->msg_lrpid = im->msg_lrpid;
	lm->msg_stime = im->msg_stime;
	lm->msg_rtime = im->msg_rtime;
	lm->msg_ctime = im->msg_ctime;
}

static void lmsq_to_imsq_l(struct msqid_ds * lm, struct ibcs_msqid_ds_l * im)
{
	memset(im, 0, sizeof(*im));
	lp_to_ip_l(&lm->msg_perm, &im->msg_perm);
	im->msg_first = lm->msg_first;
	im->msg_last = lm->msg_last;
	im->msg_cbytes = lm->msg_cbytes;
	im->msg_qnum = lm->msg_qnum;
	im->msg_qbytes = lm->msg_qbytes;
	im->msg_lspid = lm->msg_lspid;
	im->msg_lrpid = lm->msg_lrpid;
	im->msg_stime = lm->msg_stime;
	im->msg_rtime = lm->msg_rtime;
	im->msg_ctime = lm->msg_ctime;
}

int svr4_msgsys(struct pt_regs * regs)
{
	int command = get_syscall_parameter (regs, 0);
	int arg1, arg2, arg4, arg5;
	mm_segment_t old_fs;
	char *arg3;
	int retval;

	arg1 = get_syscall_parameter (regs, 1);
	arg2 = get_syscall_parameter (regs, 2);
	arg3 = (char *) get_syscall_parameter (regs, 3);

	switch (command) {
		/* hard one first */
		case U_MSGCTL:
			switch (arg2) {
				case U_IPC_SET: {
					struct ibcs_msqid_ds im;
					struct msqid_ds lm;

					retval = verify_area(VERIFY_WRITE, arg3, sizeof(im));
					if (retval)
						return retval;

					copy_from_user(&im, (char *) arg3, sizeof(im));
					imsq_to_lmsq(&im, &lm);

					old_fs = get_fs();
					set_fs (get_ds());
					retval = SYS (ipc) (MSGCTL, arg1, IPC_SET, 0, &lm);
					set_fs (old_fs);

					lmsq_to_imsq(&lm, &im);
					copy_to_user((char *)arg3, &im, sizeof(im));
					return retval;
				}

				case U_IPC_SET_L: {
					struct ibcs_msqid_ds_l im;
					struct msqid_ds lm;

					retval = verify_area(VERIFY_WRITE, arg3, sizeof(im));
					if (retval)
						return retval;

					copy_from_user(&im, (char *) arg3, sizeof(im));
					imsq_to_lmsq_l(&im, &lm);

					old_fs = get_fs();
					set_fs (get_ds());
					retval = SYS (ipc) (MSGCTL, arg1, IPC_SET, 0, &lm);
					set_fs (old_fs);

					lmsq_to_imsq_l(&lm, &im);
					copy_to_user((char *)arg3, &im, sizeof(im));
					return retval;
				}

			case U_IPC_RMID:
			case U_IPC_RMID_L:
				return SYS (ipc) (MSGCTL, arg1, IPC_RMID, 0, arg3);

			case U_IPC_STAT: {
				struct ibcs_msqid_ds im;
				struct msqid_ds lm;

				retval = verify_area(VERIFY_WRITE, arg3, sizeof(im));
				if (retval)
					return retval;

				old_fs = get_fs();
				set_fs (get_ds());
				retval = SYS (ipc) (MSGCTL, arg1, IPC_STAT, 0, &lm);
				set_fs (old_fs);

				if (retval < 0)
					return retval;

				lmsq_to_imsq(&lm, &im);
				copy_to_user((char *)arg3, &im, sizeof(im));
				return retval;
			}

			case U_IPC_STAT_L: {
				struct ibcs_msqid_ds_l im;
				struct msqid_ds lm;

				retval = verify_area(VERIFY_WRITE, arg3, sizeof(im));
				if (retval)
					return retval;

				old_fs = get_fs();
				set_fs (get_ds());
				retval = SYS (ipc) (MSGCTL, arg1, IPC_STAT, 0, &lm);
				set_fs (old_fs);

				if (retval < 0)
					return retval;

				lmsq_to_imsq_l(&lm, &im);
				copy_to_user((char *)arg3, &im, sizeof(im));
				return retval;
			}

			default:
				printk(KERN_ERR "%d ibcs_msgctl: unsupported command %d\n",
					current->pid, arg2);
		}

		case U_MSGGET:
			return SYS (ipc) (MSGGET, arg1, arg2, 0, 0);

		case U_MSGSND:
			arg4 = get_syscall_parameter (regs, 4);
			retval = SYS (ipc) (MSGSND, arg1, arg3, arg4, (char *) arg2);
			return ((retval > 0) ? 0 : retval);

		case U_MSGRCV: {
#ifdef IPCCALL
			arg4 = get_syscall_parameter (regs, 4);
			arg5 = get_syscall_parameter (regs, 5);
			return SYS(ipc)(IPCCALL(1,MSGRCV), arg1, arg3, arg5, arg2, arg4);
#else
#ifdef __sparc__
		        printk(KERN_ERR
				"%d Sparc/IBCS: Kludgy U_MSGRCV not implemented\n",
				current->pid);
			return -EINVAL;
#else /* __sparc__ */
			struct ipc_kludge *scratch;
			long old_esp = regs->esp;

			scratch = (struct ipc_kludge *)((regs->esp-1024-sizeof(struct ipc_kludge)) & 0xfffffffc);
			regs->esp = (long)scratch;
			get_user(arg4, ((unsigned long *) regs->esp) + 5);
			get_user(arg5, ((unsigned long *) regs->esp) + 6);
			put_user((long)arg2, &scratch->msgp);
			put_user((long)arg4, &scratch->msgtyp);
			retval = SYS (ipc) (MSGRCV, arg1, arg3, arg5, scratch);
			regs->esp = old_esp;
			return retval;
#endif /* sparc */
#endif /* IPCCALL */
		}

		default:
			printk(KERN_ERR "%d ibcs_msgctl: unsupported command %d\n",
				current->pid, command);
	}
	return -EINVAL;
}

EXPORT_SYMBOL(svr4_msgsys);
