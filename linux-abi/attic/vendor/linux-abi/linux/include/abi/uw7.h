#ifndef __ABI_UW7_H__
#define __ABI_UW7_H__

#ifdef __KERNEL__

#define GETACL                  1
#define SETACL                  2
#define GETACLCNT               3

struct acl { 
	int	a_type;
	uid_t	a_id;
        ushort	a_perm;
};

#define UW7_NCCS		(19)
struct uw7_termios {
	unsigned long c_iflag;
	unsigned long c_oflag;
	unsigned long c_cflag;
	unsigned long c_lflag;
	unsigned char c_cc[UW7_NCCS];
};

extern int uw7_acl(char * path, int cmd, int nentries, struct acl * aclp);

/* access.c */
extern int uw7_access(char * path, int mode);

/* kernel.c */
extern int uw7_sleep(int seconds);
extern int uw7_seteuid(int uid);
extern int uw7_setegid(int gid);
extern int uw7_pread(unsigned int fd, char * buf, int count, long off);
extern int uw7_pwrite(unsigned int fd, char * buf, int count, long off);
extern int uw7_lseek64(unsigned int fd, unsigned int off, 
	unsigned int off_hi, unsigned int orig);
extern int uw7_pread64(unsigned int fd, char * buf, int count, 
	unsigned int off_hi, unsigned int off);
extern int uw7_pwrite64(unsigned int fd, char * buf, int count, 
	unsigned int off_hi, unsigned int off);
extern int uw7_stty(int fd, int cmd);
extern int uw7_gtty(int fd, int cmd);

/* proc.c */
extern int uw7_proc_init(void);
extern void uw7_proc_cleanup(void);

/* ioctl.c */
extern int uw7_ioctl(struct pt_regs * regs);

/* mac.c */
extern int uw7_mldmode(int mldmode);

struct uw7_statvfs64 {
	unsigned long f_bsize;
	unsigned long f_frsize;
	unsigned long long f_blocks;
	unsigned long long f_bfree;
	unsigned long long f_bavail;
	unsigned long long f_files;
	unsigned long long f_ffree;
	unsigned long long f_favail;
	unsigned long f_fsid;
	char f_basetype[16];
	unsigned long f_flag;
	unsigned long f_namemax;
	char f_fstr[32];
	unsigned long f_filler[16];
};

extern int uw7_statvfs64(char * filename, struct uw7_statvfs64 * buf);
extern int uw7_fstatvfs64(int fd, struct uw7_statvfs64 * buf);

/* the other MAP_XXX values are the same as on Linux/i386 */
#define UW7_MAP_ANONYMOUS	0x100

extern int uw7_mmap(unsigned long, unsigned long, unsigned long, unsigned long, 
		unsigned long, unsigned long);

#endif /* __KERNEL__ */
#endif /* __ABI_UW7_H__ */
