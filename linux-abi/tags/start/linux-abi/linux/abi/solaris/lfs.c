/*
 *  abi/solaris/lfs.c
 *
 * $Id$
 */
#define __KERNEL__ 1

#include <linux/config.h>

#include <linux/version.h>

#include <asm/uaccess.h>

#include <linux/errno.h>
#include <linux/stat.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/file.h>
#include <linux/smp_lock.h>
#include <linux/mman.h>
#include <linux/un.h>

#include <abi/abi.h>
#include <abi/lfs.h>
#ifdef __NR_getdents
#include <linux/dirent.h>
#endif

extern unsigned short fl_ibcs_to_linux[];
extern unsigned short fl_ibcs_to_linux[];


int sol_open64(const char *fname, int flag, int mode)
{
	int error, fd, args[3];
	struct file *file;
	mm_segment_t old_fs;
	char *p;
	struct sockaddr_un addr;
#ifdef O_LARGEFILE
	fd = SYS(open)(fname, map_flags(flag, fl_ibcs_to_linux) | O_LARGEFILE, mode);
#else
	fd = SYS(open)(fname, map_flags(flag, fl_ibcs_to_linux), mode);
#endif
	if (fd < 0)
		return fd;

	/* Sometimes a program may open a pathname which it expects
	 * to be a named pipe (or STREAMS named pipe) when the
	 * Linux domain equivalent is a Unix domain socket. (e.g.
	 * UnixWare uses a STREAMS named pipe /dev/X/Nserver.0 for
	 * X :0 but Linux uses a Unix domain socket /tmp/.X11-unix/X0)
	 * It isn't enough just to make the symlink because you cannot
	 * open() a socket and read/write it. If we spot the error we can
	 * switch to socket(), connect() and things will likely work
	 * as expected however.
	 */
	file = fget(fd);
	if (!file)
		return fd; /* Huh?!? */
	if (!S_ISSOCK(file->f_dentry->d_inode->i_mode)) {
		fput(file);
		return fd;
	}
	fput(file);

	SYS(close)(fd);
	args[0] = AF_UNIX;
	args[1] = SOCK_STREAM;
	args[2] = 0;
	old_fs = get_fs();
	set_fs(get_ds());
	fd = SYS(socketcall)(SYS_SOCKET, args);
	set_fs(old_fs);
	if (fd < 0)
		return fd;

	p = getname(fname);
	if (IS_ERR(p)) {
		SYS(close)(fd);
		return PTR_ERR(p);
	}
	if (strlen(p) >= UNIX_PATH_MAX) {
		putname(p);
		SYS(close)(fd);
		return -E2BIG;
	}
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, p);
	putname(p);

	args[0] = fd;
	args[1] = (int)&addr;
	args[2] = sizeof(struct sockaddr_un);
	set_fs(get_ds());
	error = SYS(socketcall)(SYS_CONNECT, args);
	set_fs(old_fs);
	if (error) {
		SYS(close)(fd);
		return error;
	}

	return fd;
}


static int cp_sol_stat64(struct inode *inode, struct sol_stat64 * statbuf)
{
	struct sol_stat64 tmp;

	if ((unsigned long)inode->i_ino & 0xffff)
		tmp.st_ino = (unsigned long)inode->i_ino & 0xffff;
	else
		tmp.st_ino = 0xfffe;

	tmp.st_dev = inode->i_dev;
	tmp.st_mode = inode->i_mode;
	tmp.st_nlink = inode->i_nlink;
	tmp.st_uid = inode->i_uid;
	tmp.st_gid = inode->i_gid;
	tmp.st_rdev = inode->i_rdev;
	tmp.st_size = inode->i_size;
	tmp.st_atime = inode->i_atime;
	tmp.st_mtime = inode->i_mtime;
	tmp.st_ctime = inode->i_ctime;
	return copy_to_user(statbuf,&tmp,sizeof(tmp)) ? -EFAULT : 0;
}

#ifdef __cplusplus
extern "C" 
#endif
int sol_stat64(char * filename, struct sol_stat64 * statbuf)
{
	struct dentry * dentry;
	int error;

	lock_kernel();
	dentry = namei(filename);

	error = PTR_ERR(dentry);
	if (!IS_ERR(dentry)) {
		error = do_revalidate(dentry);
		if (!error)
			error = cp_sol_stat64(dentry->d_inode, statbuf);

		dput(dentry);
	}
	unlock_kernel();
	return error;
}

#ifdef __cplusplus
extern "C" 
#endif
int sol_lstat64(char * filename, struct sol_stat64 * statbuf)
{
	struct dentry * dentry;
	int error;

	lock_kernel();
	dentry = lnamei(filename);

	error = PTR_ERR(dentry);
	if (!IS_ERR(dentry)) {
		error = do_revalidate(dentry);
		if (!error)
			error = cp_sol_stat64(dentry->d_inode, statbuf);

		dput(dentry);
	}
	unlock_kernel();
	return error;
}

#ifdef __cplusplus
extern "C" 
#endif
int sol_fstat64(unsigned int fd, struct sol_stat64 * statbuf)
{
	struct file * f;
	int err = -EBADF;

	lock_kernel();
	f = fget(fd);
	if (f) {
		struct dentry * dentry = f->f_dentry;

		err = do_revalidate(dentry);
		if (!err)
			err = cp_sol_stat64(dentry->d_inode, statbuf);
		fput(f);
	}
	unlock_kernel();
	return err;
}



/* If/when the readdir function is changed to read multiple entries
 * at once this should be updated to take advantage of the fact.
 *
 * N.B. For Linux the reclen in a dirent is the number of characters
 * in the filename, for SCO (at least) reclen is the total size of
 * the particular dirent rounded up to the next multiple of 4. The SCO
 * behaviour is faithfully emulated here.
 *
 * XXXX
 * We don't truncate long filenames at all when copying. If we meet a
 * long filename and the buffer supplied by the application simply isn't
 * big enough to hold it we'll return without filling the buffer (i.e
 * return 0). The application will see this as a (premature) end of
 * directory. Is there a work around for this at all???
 */
int sol_getdents64(int fd, char *buf, int nbytes)
{
	int error, here, posn, reclen;
	struct file *file;
	struct dirent *d;
	mm_segment_t old_fs;

	error = verify_area(VERIFY_WRITE, buf, nbytes);
	if (error)
		return error;

	/* Check the file handle here. This is so we can access the current
	 * position in the file structure safely without a tedious call
	 * to sys_lseek that does nothing useful.
	 */
	file = fget(fd);
	if (!file)
		return -EBADF;

	d = (struct dirent *)__get_free_page(GFP_KERNEL);
	if (!d) {
		fput(file);
		return -ENOMEM;
	}

	error = posn = reclen = 0;
	while (posn + reclen < nbytes) {
		struct sol_dirent64 tmpbuf;
		/* Save the current position and get another dirent */
		here = file->f_pos;
		old_fs = get_fs();
		set_fs (get_ds());
		error = SYS(readdir)(fd, d, 1);
		set_fs(old_fs);
		if (error <= 0)
			break;

		/* If it'll fit in the buffer save it otherwise back up
		 * so it is read next time around.
		 * Oh, if we're at the beginning of the buffer there's
		 * no chance that this entry will ever fit so don't
		 * copy it and don't back off - we'll just pretend it
		 * isn't here...
		 */
		reclen = (sizeof(long long) + sizeof(long long)
			  + sizeof(unsigned short) + d->d_reclen + 1
			  + 3) & (~3);
		if (posn + reclen <= nbytes) {
			tmpbuf.d_off = file->f_pos;
			tmpbuf.d_ino = d->d_ino;
			tmpbuf.d_off = file->f_pos;
			tmpbuf.d_reclen = reclen;
			copy_to_user(buf+posn, &tmpbuf, 
				     sizeof(struct sol_dirent64) -1);
		        copy_to_user(buf+posn+sizeof(struct sol_dirent64)-2,
				     &d->d_name, d->d_reclen+1);
			posn += reclen;
		} else if (posn) {
			SYS(lseek)(fd, here, 0);
		} /* else posn == 0 */
	}

	/* Loose the intermediate buffer. */
	free_page((unsigned long)d);

	fput(file);

	/* If we've put something in the buffer return the byte count
	 * otherwise return the error status.
	 */
	return ((posn > 0) ? posn : error);
}


int sol_mmap64(unsigned vaddr, unsigned vsize, int prot, int flags,
	   int fd, unsigned int off_hi, unsigned file_offset)
{
	int error;
	struct file * file = NULL;

	if (off_hi != 0)
		return -EINVAL;

	if (!(flags & MAP_ANONYMOUS)) {
		if (!(file = fget(fd)))
			return -EBADF;
	}
	if (!(flags & 0x80000000) && vaddr) {
		unsigned int ret;
		ret = do_mmap(file, vaddr, vsize, prot, flags | MAP_FIXED, file_offset);
		if (file) fput(file);
		return ret;
	}

	error = do_mmap(file, vaddr, vsize, prot, flags & 0x7fffffff, file_offset);
	if (file) fput(file);

	return error;
}
