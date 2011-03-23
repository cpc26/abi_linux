/*
 *  These are defined to enable their inclsion in the branch table
 *  defined below.
 *
 *  Hacked by Eric Youngdale for iBCS (1993, 1994).
 *
 * $Id$
 * $Source$
 */

#ifndef __ABI_ABI_H__
#define __ABI_ABI_H__
#include <abi/svr4sig.h>

typedef unsigned long ABI_dev_t;
typedef unsigned long ABI_ino_t;
typedef unsigned long ABI_mode_t;
typedef unsigned long ABI_nlink_t;
typedef long ABI_uid_t;
typedef long ABI_off_t;
typedef struct timeval ABI_timestruc_t;


struct abi_sigaction {
       int          sa_flags;
       __sighandler_t sa_handler;
       unsigned long sa_mask;
       int	    sa_resv[2];  /* Reserved for something or another */
};
#define ABI_SA_ONSTACK   1
#define ABI_SA_RESETHAND 2
#define ABI_SA_RESTART   4
#define ABI_SA_SIGINFO   8
#define ABI_SA_NODEFER  16
#define ABI_SA_NOCLDWAIT 0x10000
#define ABI_SA_NOCLDSTOP 0x20000


struct sco_sigaction {
	void		(*sa_handler)(int);
	unsigned long	sa_mask;
	int		sa_flags;
};
#define SCO_SA_NOCLDSTOP	0x001
#define SCO_SA_COMPAT		0x080 /* 3.2.2 compatibilty. Like SA_SIGNAL? */
#define SCO_SA_SIGNAL		0x100


extern int abi_sigaction(int abi_signum, const struct abi_sigaction * action,
	struct abi_sigaction * oldaction);
extern int sco_sigaction(int sco_signum, const struct sco_sigaction * action,
	struct sco_sigaction * oldaction);
extern int abi_sigprocmask(int how, unsigned long *set, unsigned long *oset);
extern int abi_sigsuspend(struct pt_regs * regs);


/*
 * This is the general form of the stat structure in an ABI compliant system.
 */

struct	svr4_xstat {
	ABI_dev_t	st_dev;
	long	st_pad1[3];	 
	ABI_ino_t	st_ino;
	ABI_mode_t	st_mode;
	ABI_nlink_t st_nlink;
	ABI_uid_t 	st_uid;
	ABI_uid_t 	st_gid;
	ABI_dev_t	st_rdev;
	long	st_pad2[2];
	ABI_off_t	st_size;
	long	st_pad3;	 
	ABI_timestruc_t st_atim;	
	ABI_timestruc_t st_mtim;	
	ABI_timestruc_t st_ctim;	
	long	st_blksize;
	long	st_blocks;
	char	st_fstype[ 16 ] ;
	long	st_pad4[8];
};

/*
 * Structure used by statvfs syscall.
 */
struct abi_statvfs {
  unsigned long f_bsize; /* blocksize */
  unsigned long f_frsize; /* fragment size. */
  unsigned long f_blocks;
  unsigned long f_bfree;
  unsigned long f_bavail;
  unsigned long f_files;
  unsigned long f_free;
  unsigned long f_sid;
  char	        f_basetype[16];
  unsigned long f_flag;
  unsigned long f_namemax;
  char		f_fstr[32];
  unsigned long f_filler[16];
};

struct sco_statvfs {
  unsigned long f_bsize; /* blocksize */
  unsigned long f_frsize; /* fragment size. */
  unsigned long f_blocks;
  unsigned long f_bfree;
  unsigned long f_bavail;
  unsigned long f_files;
  unsigned long f_free;
  unsigned long f_favail;
  unsigned long f_sid;
  char	        f_basetype[16];
  unsigned long f_flag;
  unsigned long f_namemax;
  char		f_fstr[32];
  unsigned long f_filler[16];
};

extern int svr4_xstat(int vers, char * path, void * buf);
extern int svr4_lxstat(int vers, char * path, void * buf);
extern int svr4_xmknod(int vers, const char * path, mode_t mode, dev_t dev);

extern int abi_statvfs(char * path, void * buf);
extern int abi_fstatvfs(int fd, void * buf);



int svr4_fpathconf(int fd, int name);
int svr4_pathconf(char *path, int name);
int svr4_sigpending (int which_routine, svr4_sigset_t *set);
int svr4_context (struct pt_regs *regs);
int svr4_ioctl(struct pt_regs *regs);

/* POSIX.1 names */
#define _PC_LINK_MAX    1
#define _PC_MAX_CANON   2
#define _PC_MAX_INPUT   3
#define _PC_NAME_MAX    4
#define _PC_PATH_MAX    5
#define _PC_PIPE_BUF    6
#define _PC_NO_TRUNC    7
#define _PC_VDISABLE    8
#define _PC_CHOWN_RESTRICTED    9
/* POSIX.4 names */
#define _PC_ASYNC_IO    10
#define _PC_PRIO_IO     11
#define _PC_SYNC_IO     12
#endif
