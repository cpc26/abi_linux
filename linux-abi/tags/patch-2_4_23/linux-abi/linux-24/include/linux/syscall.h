#ifndef _LINUX_SYSCALL_H
#define _LINUX_SYSCALL_H

/*
 * Prototypes for Linux syscalls.
 *
 * Maybe this could be automatically generated from some kind of
 * master file (like BSD's syscalls.master), so it is always coherent
 * to the actual syscalls (which may as well be arch-specific).
 */

#include <linux/sched.h>	/* struct pt_regs */
#include <asm/signal.h>		/* old_sigset_t */

#include <asm/syscall.h>


struct itimerval;
struct msghdr;
struct pollfd;
struct rlimit;
struct sigaction;
struct sockaddr;
struct statfs;
struct timespec;
struct timeval;
struct timezone;
struct tms;
struct utimbuf;


/* fs/exec.c */
extern asmlinkage long sys_uselib(const char * library);

/* fs/buffer.c */
extern asmlinkage long sys_sync(void);
extern asmlinkage long sys_fsync(unsigned int fd);
extern asmlinkage long sys_fdatasync(unsigned int fd);

/* fs/fcntl.c */
extern asmlinkage long sys_dup(unsigned int fildes);
extern asmlinkage long sys_dup2(unsigned int oldfd, unsigned int newfd);
extern asmlinkage long sys_fcntl(unsigned int fd, unsigned int cmd,
		unsigned long arg);
#if (BITS_PER_LONG == 32)
extern asmlinkage long sys_fcntl64(unsigned int fd, unsigned int cmd,
		unsigned long arg);
#endif

/* fs/ioctl.c */
extern asmlinkage long sys_ioctl(unsigned int fd, unsigned int cmd, void *);

/* fs/namei.c */
extern asmlinkage long sys_link(const char * oldname, const char * newname);
extern asmlinkage long sys_mkdir(const char * pathname, int mode);
extern asmlinkage long sys_mknod(const char * filename, int mode, dev_t dev);
extern asmlinkage long sys_rename(const char * oldname, const char * newname);
extern asmlinkage long sys_rmdir(const char * pathname);
extern asmlinkage long sys_symlink(const char * oldname, const char * newname);
extern asmlinkage long sys_unlink(const char * pathname);

/* fs/namespace.c */
extern asmlinkage long sys_umount(char * name, int flags);
extern asmlinkage long sys_oldumount(char * name);
extern asmlinkage long sys_mount(char * dev_name, char * dir_name,
		char * type, unsigned long flags, void * data);
extern asmlinkage long sys_pivot_root(const char *new_root,
		const char *put_old);

/* fs/open.c */
extern asmlinkage long sys_access(const char * filename, int mode);            
extern asmlinkage long sys_chdir(const char * filename);
extern asmlinkage long sys_fchdir(unsigned int fd);
extern asmlinkage long sys_chroot(const char * filename);
extern asmlinkage long sys_chmod(const char * filename, mode_t mode);
extern asmlinkage long sys_fchmod(unsigned int fd, mode_t mode);
extern asmlinkage long sys_chown(const char * filename, uid_t user,
		gid_t group);
extern asmlinkage long sys_lchown(const char * filename, uid_t user,
		gid_t group);
extern asmlinkage long sys_fchown(unsigned int fd, uid_t user, gid_t group);
extern asmlinkage long sys_close(unsigned int fd);
#if !defined(__alpha__)
extern asmlinkage long sys_creat(const char * pathname, int mode);
#endif
extern asmlinkage long sys_open(const char * filename, int flags, int mode);
extern asmlinkage long sys_statfs(const char * path, struct statfs * buf);
extern asmlinkage long sys_fstatfs(unsigned int fd, struct statfs * buf);
extern asmlinkage long sys_ftruncate(unsigned int fd, unsigned long length);
extern asmlinkage long sys_ftruncate64(unsigned int fd, loff_t length);
extern asmlinkage long sys_truncate(const char * path, unsigned long length);
extern asmlinkage long sys_truncate64(const char * path, loff_t length);
#if !defined(__alpha__) && !defined(__ia64__)
extern asmlinkage long sys_utime(char * filename, struct utimbuf * times);
#endif
extern asmlinkage long sys_utimes(char * filename, struct timeval * utimes);
extern asmlinkage long sys_vhangup(void);


/* fs/read_write.c */
extern asmlinkage off_t	sys_lseek(unsigned int fd, off_t offset,
		unsigned int origin);
#if !defined(__alpha__)
extern asmlinkage long sys_llseek(unsigned int fd, unsigned long offset_high,
		unsigned long offset_low, loff_t * result, unsigned int origin);
#endif
extern asmlinkage ssize_t sys_read(unsigned int fd, char * buf, size_t count);
extern asmlinkage ssize_t sys_readv(unsigned long fd,
		const struct iovec * vector, unsigned long count);
extern asmlinkage ssize_t sys_pread(unsigned int fd, char * buf,
		size_t count, loff_t pos);
extern asmlinkage ssize_t sys_pwrite(unsigned int fd, const char * buf,
		size_t count, loff_t pos);
extern asmlinkage ssize_t sys_write(unsigned int fd, const char * buf,
		size_t count);
extern asmlinkage ssize_t sys_writev(unsigned long fd,
		const struct iovec * vector, unsigned long count);



/* fs/readdir.c */
extern asmlinkage int old_readdir(unsigned int fd, void * dirent,
				unsigned int count);

/* fs/select.c */
extern asmlinkage long sys_poll(struct pollfd * ufds, unsigned int nfds,
				long timeout);
extern asmlinkage int sys_select(int, fd_set *, fd_set *, fd_set *,
				struct timeval *);

/* fs/stat.c */
extern asmlinkage long sys_readlink(const char * path, char * buf,
				int bufsiz);

/* fs/super.c */
extern asmlinkage long sys_sysfs(int option, unsigned long arg1,
				unsigned long arg2);

/* kernel/acct.c */
extern asmlinkage long sys_acct(const char *name);

/* kernel/exit.c */
extern asmlinkage long sys_exit(int error_code);
extern asmlinkage long sys_waitpid(pid_t pid, unsigned int *stat_addr,
		int options);

/* kernel/itimer.c */
extern asmlinkage long sys_getitimer(int which, struct itimerval *value);
extern asmlinkage long sys_setitimer(int which, struct itimerval *value,
				struct itimerval *ovalue);

/* kernel/sched.c */
#if !defined(__alpha__)
extern asmlinkage long sys_nice(int increment);
#endif
extern asmlinkage long sys_sched_setscheduler(pid_t pid, int policy,
		struct sched_param *param);
extern asmlinkage long sys_sched_setparam(pid_t pid,
		struct sched_param *param);
extern asmlinkage long sys_sched_getscheduler(pid_t pid);
extern asmlinkage long sys_sched_getparam(pid_t pid,
		struct sched_param *param);
extern asmlinkage long sys_sched_yield(void);
extern asmlinkage long sys_sched_get_priority_max(int policy);
extern asmlinkage long sys_sched_get_priority_min(int policy);
extern asmlinkage long sys_sched_rr_get_interval(pid_t pid,
		struct timespec *interval);

/* kernel/signal.c */
extern asmlinkage long sys_kill(int pid, int sig);
extern asmlinkage long sys_rt_sigaction(int sig, const struct sigaction *act,
				struct sigaction *oact, size_t sigsetsize);
extern asmlinkage long sys_rt_sigpending(sigset_t *set, size_t sigsetsize);
extern asmlinkage long sys_rt_sigprocmask(int how, sigset_t *set,
				sigset_t *oset, size_t sigsetsize);
extern asmlinkage long sys_rt_sigtimedwait(const sigset_t *uthese,
				siginfo_t *uinfo, const struct timespec *uts,
				size_t sigsetsize);
extern asmlinkage long sys_sigaltstack(const stack_t *uss, stack_t *uoss);
extern asmlinkage long sys_sigpending(old_sigset_t *set);
extern asmlinkage long sys_sigprocmask(int how, old_sigset_t *set,
				old_sigset_t *oset);
extern asmlinkage int sys_sigsuspend(int history0, int history1,
				old_sigset_t mask);

/* kernel/sys.c */
extern asmlinkage long sys_gethostname(char *name, int len);
extern asmlinkage long sys_sethostname(char *name, int len);
extern asmlinkage long sys_setdomainname(char *name, int len);
extern asmlinkage long sys_getrlimit(unsigned int resource,
				struct rlimit *rlim);
extern asmlinkage long sys_setsid(void);
extern asmlinkage long sys_getsid(pid_t pid);
extern asmlinkage long sys_getpgid(pid_t pid);
extern asmlinkage long sys_setpgid(pid_t pid, pid_t pgid);
extern asmlinkage long sys_getgroups(int gidsetsize, gid_t *grouplist);
extern asmlinkage long sys_setgroups(int gidsetsize, gid_t *grouplist);
extern asmlinkage long sys_setpriority(int which, int who, int niceval);
extern asmlinkage long sys_getpriority(int which, int who);
extern asmlinkage long sys_reboot(int magic1, int magic2, unsigned int cmd,
		void * arg);
extern asmlinkage long sys_setgid(gid_t gid);
extern asmlinkage long sys_setuid(uid_t uid);
extern asmlinkage long sys_times(struct tms * tbuf);
extern asmlinkage long sys_umask(int mask);
extern asmlinkage long sys_prctl(int option, unsigned long arg2,
		unsigned long arg3, unsigned long arg4, unsigned long arg5);
extern asmlinkage long sys_setreuid(uid_t ruid, uid_t euid);
extern asmlinkage long sys_setregid(gid_t rgid, gid_t egid);

/* kernel/time.c */
extern asmlinkage long sys_gettimeofday(struct timeval *tv,
				struct timezone *tz);
extern asmlinkage long sys_settimeofday(struct timeval *tv,
				struct timezone *tz);
extern asmlinkage long sys_stime(int * tptr);
extern asmlinkage long sys_time(int * tloc);

/* kernel/timer.c */
#if !defined(__alpha__) && !defined(__ia64__)
extern asmlinkage unsigned long sys_alarm(unsigned int seconds);
#endif
#if !defined(__alpha__)
extern asmlinkage long sys_getpid(void);
extern asmlinkage long sys_getppid(void);
extern asmlinkage long sys_getuid(void);
extern asmlinkage long sys_geteuid(void);
extern asmlinkage long sys_getgid(void);
extern asmlinkage long sys_getegid(void);
#endif
extern asmlinkage long sys_gettid(void);
extern asmlinkage long sys_nanosleep(struct timespec *rqtp,
		struct timespec *rmtp);

#if defined(CONFIG_UID16)
/* kernel/uid16.c */
extern asmlinkage long sys_setreuid16(old_uid_t ruid, old_uid_t euid);
extern asmlinkage long sys_setregid16(old_gid_t rgid, old_gid_t egid);
extern asmlinkage long sys_getgroups16(int gidsetsize, old_gid_t *grouplist);
extern asmlinkage long sys_setgroups16(int gidsetsize, old_gid_t *grouplist);
#endif /* CONFIG_UID16 */

/* mm/filemap.c */
extern asmlinkage ssize_t sys_sendfile(int out_fd, int in_fd, off_t *offset,
		size_t count);
extern asmlinkage long sys_msync(unsigned long start, size_t len, int flags);
extern asmlinkage long sys_madvise(unsigned long start, size_t len,
		int behavior);
extern asmlinkage long sys_mincore(unsigned long start, size_t len,
		unsigned char * vec);

/* mm/mmap.c */
extern asmlinkage unsigned long sys_brk(unsigned long brk);
extern asmlinkage long sys_munmap(unsigned long addr, size_t len);

/* mm/mprotect.c */
extern asmlinkage long sys_mprotect(unsigned long start, size_t len,
		unsigned long prot);

/* net/socket.c */
extern asmlinkage long sys_socket(int family, int type, int protocol);
extern asmlinkage long sys_socketpair(int family, int type,
				int protocol, int usockvec[2]);
extern asmlinkage long sys_bind(int fd, struct sockaddr *umyaddr,
				int addrlen);
extern asmlinkage long sys_listen(int fd, int backlog);
extern asmlinkage long sys_accept(int fd, struct sockaddr *upeer_sockaddr,
				int *upeer_addrlen);
extern asmlinkage long sys_connect(int fd, struct sockaddr *uservaddr,
				int addrlen);
extern asmlinkage long sys_getsockname(int fd, struct sockaddr *usockaddr,
				int *usockaddr_len);
extern asmlinkage long sys_getpeername(int fd, struct sockaddr *usockaddr,
				int *usockaddr_len);
extern asmlinkage long sys_sendto(int fd, void * buff, size_t len,
				unsigned flags, struct sockaddr *addr,
				int addr_len);
extern asmlinkage long sys_send(int fd, void * buff, size_t len,
					unsigned flags);
extern asmlinkage long sys_recvfrom(int fd, void * ubuf, size_t size,
				unsigned flags, struct sockaddr *addr,
				int *addr_len);
extern asmlinkage long sys_setsockopt(int fd, int level, int optname,
				char *optval, int optlen);
extern asmlinkage long sys_getsockopt(int fd, int level, int optname,
				char *optval, int *optlen);
extern asmlinkage long sys_shutdown(int fd, int how);
extern asmlinkage long sys_sendmsg(int fd, struct msghdr *msg,
				unsigned flags);
extern asmlinkage long sys_recvmsg(int fd, struct msghdr *msg,
				unsigned int flags);
extern asmlinkage long sys_socketcall(int call, unsigned long *args);

#endif /* _LINUX_SYSCALL_H */
