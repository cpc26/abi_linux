/* Large File Summit support functions */

#ifndef __IBCS_LFS_H__

#include <abi/abi.h>
#include <abi/svr4sig.h>

struct  sol_stat64 {
        unsigned long		st_dev;
        long			st_pad1[3];     /* reserve for dev expansion */
                                /* sysid definition */
        unsigned long long	st_ino;
        unsigned long		st_mode;
        unsigned long		st_nlink;
        unsigned long		st_uid;
        unsigned long		st_gid;
        unsigned long		st_rdev;
        long			st_pad2[2];
        unsigned long long	st_size;        /* large file support */
        unsigned long		st_atime;
        unsigned long		st_mtime;
        unsigned long		st_ctime;
        long			st_blksize;
        long long		st_blocks;   /* large file support */
        char			st_fstype[16];
        long			st_pad4[8];     /* expansion area */
};

struct sol_dirent64 {
	unsigned long long d_ino;
	unsigned long long d_off;
	unsigned short d_reclen;
	char d_name[1];
};

int sol_stat64(char * filename, struct sol_stat64 * statbuf);
int sol_lstat64(char * filename, struct sol_stat64 * statbuf);
int sol_fstat64(unsigned int fd, struct sol_stat64 * statbuf);
int sol_open64(const char *fname, int flag, int mode);
int sol_getdents64(int fd, char *buf, int nbytes);
int sol_mmap64(unsigned vaddr, unsigned vsize, int prot, int flags,
	   int fd, unsigned int off_hi, unsigned file_offset);
	
/* version 4 (UW7_STAT64_VERSION) stat structure */
struct uw7_stat64 {
	unsigned long		st_dev;
	long			st_pad1[3];
	unsigned long long	st_ino;
	unsigned long		st_mode;
	unsigned long		st_nlink;
	long			st_uid;
	long			st_gid;
	unsigned long		st_rdev;
	long			st_pad2[2];
	long long		st_size;
	struct timeval		st_atime;
	struct timeval		st_mtime;
	struct timeval		st_ctime;
	long			st_blksize;
	long long		st_blocks;
	char			st_fstype[16];
	int			st_aclcnt;
	unsigned long		st_level;
	unsigned long		st_flags;	/* may contain MLD flag */
	unsigned long		st_cmwlevel;
	long			st_pad4[4];
};

#endif /* __IBCS_LFS_H__ */
