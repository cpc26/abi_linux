/*
 *  linux/abi/svr4_common/ioctl.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *
 *  Written by Drew Sullivan.
 *  Rewritten by Mike Jagdis.
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
#include <linux/stat.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/termios.h>
#include <linux/mtio.h>
#include <linux/time.h>
#include <linux/sockios.h>
#include <linux/mm.h>
#include <linux/file.h>

#include <abi/abi.h>
#include <abi/bsd.h>
#include <abi/stream.h>
#include <abi/tli.h>
#include <abi/svr4.h>
#include <abi/compat.h>

#ifdef CONFIG_ABI_TRACE
#include <abi/trace.h>
#endif


#ifdef CONFIG_ABI_IBCS_SCO
static int sco_ioctl_tape(int fd, unsigned int func, unsigned long arg);
#endif
static int bsd_ioctl_file(int fd, unsigned int func, void *arg);
static int ibcs_ioctl_tape(int fd, unsigned int func, unsigned long arg);
static int ibcs_ioctl_termios(int fd, unsigned int func, void *arg);
static int ibcs_ioctl_termio(int fd, unsigned int func, void *arg);
extern int abi_ioctl_console(int fd, unsigned int func, void *arg);
extern int abi_ioctl_video(int fd, unsigned int func, void *arg);

/* do_ioctl() is a meta mapper, that is
   it looks up the class of the ioctl and then
   dispatchs to lower level routines to handle the
   mapping of the actual ioctls
*/

static int
do_ioctl(struct pt_regs *regs, int fd, unsigned long ioctl_num, void *arg)
{
	unsigned int class = ioctl_num >> 8;
	char class_str[4];

	switch (class) {
		case 0: /* SCO ioctls on the pseudo NFS device probably. */
			return abi_ioctl_socksys(fd, ioctl_num, arg);

		case 'A': /* ??? SCO console keyboard stuff? */
			return -EINVAL;

#ifdef CONFIG_ABI_IBCS_SCO
		case 'm':
			return sco_ioctl_tape(fd, ioctl_num, (unsigned long)arg);
#endif
		case 't':
			if (personality(PER_SVR4))
				return ibcs_ioctl_tape(fd, ioctl_num, (unsigned long)arg);
			else
				return bsd_ioctl_termios(fd, ioctl_num, arg);

		case 'f':
			return bsd_ioctl_file(fd, ioctl_num, arg);

		case 'T':	/* xenix ioctl compatibility */
			return ibcs_ioctl_termio(fd, ioctl_num & 0xFF, arg);

		case ('i' << 16) | ('X' << 8):	/* iBCS2 POSIX */
		case 'x':	/* Pre-iBCS2 POSIX */
			return ibcs_ioctl_termios(fd, ioctl_num & 0xFF, arg);

		case 'C':
		case 'c':
			return abi_ioctl_console(fd, ioctl_num, arg);

		case ('i' << 16) | ('C' << 8):	/* iBCS2 POSIX */
			return abi_ioctl_video(fd, ioctl_num & 0xFF, arg);

		/* SCO 3.2.2 uses ('X'<<8)|1 for access to the video map
		 * and the 'X' set is also used for synchronous comm
		 * lines (I think?). SVR4 uses it for tty extensions to
		 * support hardware flow control and external clocks.
		 */
		case 'X':
			if (personality(PER_SVR4) || personality(PER_SOLARIS))
				return svr4_ioctl_termiox(fd, ioctl_num & 0xFF, arg);
			return -EINVAL;

		/* These aren't implemented and are never likely to be as they
		 * are specific to drivers for obscure hardware. (For those
		 * that don't know they're the JERQ ioctls. Says it all
		 * really!)
		 */
		case 'j':
			return -EINVAL;

		/* The 'S' set could also be display mode switch
		 * ioctls in a SCO 3.2.x x<4 environment. It should
		 * depend on the descriptor they are applied to.
		 * According to ISC the Xenix STREAMS ioctls had the
		 * high bit set on the command to differentiate them
		 * from mode switch ioctls. Yuk, yuk, yuk...
		 */
		case 'S':
			return svr4_ioctl_stream(regs, fd, ioctl_num & 0x7F, arg);

		/* These are STREAMS socket module ioctls. */
		case 'I':
#if defined(CONFIG_ABI_XTI)
			return svr4_ioctl_sockmod(fd, ioctl_num & 0xFF, arg);
#else
			return -EINVAL;
#endif

		/* These are SCO <vtkd.h> ioctls - see vtkd.h */
	        case 'v':
	        case 'K':
		        return ibcs_ioctl_vtkd(fd, ioctl_num, arg);

		/* EUC ioctls. These are something to do with chararcter
		 * code set conversions in SVR4. If we don't support
		 * them the correct thing to do is to return EINVAL.
		 */
		case 'E'|0x80:
			return -EINVAL;

		/* SCO channel mapping. I can't find any documentation
		 * for this. These are the LD?MAP ioctls defined in
		 * sys/termio.h and sys/emap.h. They are used by mapchan.
		 */
		case 'D':
			return -EINVAL;
	}

	/* If we haven't handled it yet it must be a BSD style ioctl
	 * with a (possible) argument description in the high word of
	 * the opcode.
	 */
	switch (class & 0xff) {
		/* From SVR4 as specified in sys/iocomm.h */
		case 'f':
			return bsd_ioctl_file(fd, ioctl_num, arg);

		/* BSD or V7 terminal ioctls. */
		case 't':
			return bsd_ioctl_termios(fd, ioctl_num, arg);

		/* "Traditional" BSD and Wyse V/386 3.2.1A TCP/IP ioctls. */
		case 's':
		case 'r':
		case 'i':
			return abi_ioctl_socksys(fd, ioctl_num, arg);

		/* SVR3 streams based socket TCP/IP ioctls.
		 * These are handed over to the standard ioctl
		 * handler since /dev/socksys is an emulated device
		 * and front ends any sockets created through it.
		 * Note that 'S' ioctls without the BSDish argument
		 * type in the high bytes are STREAMS ioctls and 'I'
		 * ioctls without the BSDish type in the high bytes
		 * are the STREAMS socket module ioctls. (see above).
		 */
		case 'S':
		case 'R':
		case 'I':
			return abi_ioctl_socksys(fd, ioctl_num, arg);
	}

	/* If nothing has handled it yet someone may have to do some
	 * more work...
	 */
	class_str[0] = class & 0xFF0000 ? (char)((class >> 16) & 0xFF) : '.';
	class_str[1] = class & 0x00FF00 ? (char)((class >>  8) & 0xFF) : '.';
	class_str[2] = class & 0x0000FF ? (char)((class      ) & 0xFF) : '.';
	class_str[3] = 0;

	printk(KERN_ERR "iBCS: ioctl(%d, %lx[%s], 0x%lx) unsupported\n",
		fd, ioctl_num, class_str, (unsigned long)arg);

	return -EINVAL;
}


/* Some of these are used by SVR3/4 too... */
static int bsd_ioctl_file(int fd, unsigned int func, void *arg)
{
	switch (func) {
		case BSD__IOV('f', 1): case BSD__IO('f', 1): /* FIOCLEX */
			FD_SET(fd, CLOSE_ON_EXEC_FDS_ADDR);
			return 0;

		case BSD__IOV('f', 2): case BSD__IO('f', 2): /* FIONCLEX */
			FD_CLR(fd, CLOSE_ON_EXEC_FDS_ADDR);
			return 0;

		case BSD__IOV('f', 3): case BSD__IO('f', 3): { /* FIORDCHK */
			int error, nbytes;
			mm_segment_t old_fs;

			old_fs = get_fs();
			set_fs (get_ds());
			error = SYS(ioctl)(fd, FIONREAD, &nbytes);
			set_fs(old_fs);

			return (error <= 0 ? error : nbytes);
		}

		case BSD__IOW('f', 123, int): /* FGETOWN */
			return SYS(ioctl)(fd, FIOGETOWN, arg);

		case BSD__IOW('f', 124, int): /* FSETOWN */
			return SYS(ioctl)(fd, FIOSETOWN, arg);

		case BSD__IOW('f', 125, int): /* FIOASYNC */
			return SYS(ioctl)(fd, FIOASYNC, arg);

		case BSD__IOW('f', 126, int): /* FIONBIO */
			return SYS(ioctl)(fd, FIONBIO, arg);

		case BSD__IOR('f', 127, int): /* FIONREAD */
			return SYS(ioctl)(fd, FIONREAD, arg);
	}

	printk(KERN_ERR "iBCS: file ioctl 0x%08lx unsupported\n",
		(unsigned long)func);
	return -EINVAL;
}

static int svr_to_linux_termio(int fd, int op, struct svr_termio *it)
{
	struct termio t;
	mm_segment_t old_fs;
	char eof;
	unsigned short lflag;
	int error;

	error = verify_area(VERIFY_READ, it, sizeof(struct svr_termio));
	if (error)
		return error;

	old_fs = get_fs();
	set_fs(get_ds());
	error = SYS(ioctl)(fd, TCGETA, &t);
	set_fs(old_fs);
	if (error)
		return error;

	/* Save things we may need later. */
	eof = t.c_cc[4];
	lflag = t.c_lflag;

	/* Copy the entire structure then fix up as necessary. */
	copy_from_user(&t, it, sizeof(struct svr_termio));

	/* If ICANON isn't set then we've been given VMIN in place
	 * of VEOF.
	 */
	if (!(t.c_lflag & 0000002)) {
		t.c_cc[6] = t.c_cc[4];
		t.c_cc[4] = eof;
	}

	if (t.c_cflag & 0100000) /* CRTSFL - SCO only? */
		t.c_cflag |= CRTSCTS;
	t.c_cflag &= ~0170000; /* LOBLK|CTSFLOW|RTSFLOW|CRTSFL */

	set_fs(get_ds());
	error = SYS(ioctl)(fd, op, &t);
	set_fs(old_fs);

	return error;
}

static int linux_to_svr_termio(int fd, struct svr_termio *it)
{
	struct termio t;
	mm_segment_t old_fs;
	int error;

	error = verify_area(VERIFY_WRITE, it, sizeof(struct svr_termio));
	if (error)
		return error;

	old_fs = get_fs();
	set_fs(get_ds());
	error = SYS(ioctl)(fd, TCGETA, &t);
	set_fs(old_fs);
	if (error)
		return error;

	/* If ICANON isn't set then we substitute VEOF with VMIN. */
	if (!(t.c_lflag & 0000002)) {
		t.c_cc[4] = t.c_cc[6];
	}

	/* Copy to the user supplied structure. */
	copy_to_user(it, &t, sizeof(struct svr_termio));

	return error;
}


#ifdef CONFIG_ABI_IBCS_SCO
#define SCO_NCCS (SVR_NCC+5)
struct sco_termios {
	unsigned short c_iflag;
	unsigned short c_oflag;
	unsigned short c_cflag;
	unsigned short c_lflag;
	char c_line;
	unsigned char c_cc[SCO_NCCS];
	char c_ispeed;
	char c_ospeed;
};
static int sco_to_linux_termios(int fd, int op, struct sco_termios *it)
{
	struct termios t;
	mm_segment_t old_fs;
	unsigned short lflag, r;
	char sco_cc[SCO_NCCS];
	int error;

	error = verify_area(VERIFY_READ, it, sizeof(struct sco_termios));
	if (error)
		return error;

	old_fs = get_fs();
	set_fs(get_ds());
	error = SYS(ioctl)(fd, TCGETS, &t);
	set_fs(old_fs);
	if (error)
		return error;

	__get_user(t.c_iflag, &it->c_iflag);
	t.c_iflag &= ~0100000; /* DOSMODE */

	__get_user(t.c_oflag, &it->c_oflag);

	__get_user(t.c_cflag, &it->c_cflag);
	if (t.c_cflag & 0100000) /* CRTSFL - SCO only? */
		t.c_cflag |= CRTSCTS;
	t.c_cflag &= ~0170000; /* LOBLK|CTSFLOW|RTSFLOW|CRTSFL */

	lflag = t.c_lflag;
	t.c_lflag &= ~0100777;
	__get_user(r, &it->c_lflag);
	t.c_lflag |= r;
	if ((t.c_lflag & 0100000))
		SYS(ioctl)(fd, TIOCEXCL, 0);
	else
		SYS(ioctl)(fd, TIOCNXCL, 0);
	t.c_lflag &= ~0100000;
	t.c_lflag |= (t.c_lflag & 0000400) << 7; /* Move IEXTEN */
	t.c_lflag &= ~0000400;
	t.c_lflag |= (t.c_lflag & 0001000) >> 1; /* Move TOSTOP */
	t.c_lflag &= ~0001000;
	t.c_lflag |= (lflag & 0001000); /* Restore ECHOCTL */

	__get_user(t.c_line, &it->c_line); /* XXX Map this? */

	copy_from_user(sco_cc, &it->c_cc, SCO_NCCS);
	t.c_cc[0] = sco_cc[0];
	t.c_cc[1] = sco_cc[1];
	t.c_cc[2] = sco_cc[2];
	t.c_cc[3] = sco_cc[3];
	t.c_cc[7] = sco_cc[7];
	t.c_cc[8] = sco_cc[11];
	t.c_cc[9] = sco_cc[12];
	t.c_cc[10] = sco_cc[10];
	t.c_cc[16] = sco_cc[6];
	if (t.c_lflag & ICANON) {
		t.c_cc[4] = sco_cc[4];
		t.c_cc[11] = sco_cc[5];
	} else {
		t.c_cc[4] = sco_cc[8];
		t.c_cc[5] = sco_cc[5];
		t.c_cc[6] = sco_cc[4];
		t.c_cc[11] = sco_cc[9];
	}

	set_fs(get_ds());
	error = SYS(ioctl)(fd, op, &t);
	set_fs(old_fs);

	return error;
}

static int linux_to_sco_termios(int fd, int op, struct sco_termios *it)
{
	struct termios t;
	char sco_cc[SCO_NCCS];
	mm_segment_t old_fs;
	int error;

	error = verify_area(VERIFY_WRITE, it, sizeof(struct sco_termios));
	if (error)
		return error;

	old_fs = get_fs();
	set_fs(get_ds());
	error = SYS(ioctl)(fd, op, &t);
	set_fs(old_fs);
	if (error)
		return error;

	put_user(t.c_iflag & 0017777, &it->c_iflag);

	put_user(t.c_oflag & 0177777, &it->c_oflag);

	if (t.c_cflag & CRTSCTS)
		t.c_cflag |= 0100000; /* CRTSFL - SCO only? */
	put_user(t.c_cflag & 0177777, &it->c_cflag);

	t.c_lflag &= ~0001000;
	t.c_lflag |= (t.c_lflag & 0000400) << 1;
	t.c_lflag &= ~0000400;
	t.c_lflag |= (t.c_lflag & 0100000) >> 7;
	t.c_lflag &= ~0100000;
	put_user(t.c_lflag & 0001777, &it->c_lflag);

	put_user(t.c_line, &it->c_line); /* XXX Map this? */

	sco_cc[0] = t.c_cc[0];
	sco_cc[1] = t.c_cc[1];
	sco_cc[2] = t.c_cc[2];
	sco_cc[3] = t.c_cc[3];
	sco_cc[6] = t.c_cc[16];
	sco_cc[7] = t.c_cc[7];
	sco_cc[8] = t.c_cc[4];
	sco_cc[9] = t.c_cc[11];
	sco_cc[10] = t.c_cc[10];
	sco_cc[11] = t.c_cc[8];
	sco_cc[12] = t.c_cc[9];
	if (t.c_lflag & ICANON) {
		sco_cc[4] = t.c_cc[4];
		sco_cc[5] = t.c_cc[11];
	} else {
		sco_cc[4] = t.c_cc[6];
		sco_cc[5] = t.c_cc[5];
	}

	copy_to_user(&it->c_cc, sco_cc, SCO_NCCS);

	return error;
}
#endif /* CONFIG_ABI_IBCS_SCO */


/* XXX This is just copied from SCO above. There are minor differences
 * but not in any of the critical flags and non of them overlap - I think.
 */

static int svr4_to_linux_termios(int fd, int op, struct svr4_termios *it)
{
	struct termios t;
	mm_segment_t old_fs;
	unsigned short lflag, r;
	char svr4_cc[SVR4_NCCS];
	int error;

	error = verify_area(VERIFY_READ, it, sizeof(struct svr4_termios));
	if (error)
		return error;

	old_fs = get_fs();
	set_fs(get_ds());
	error = SYS(ioctl)(fd, TCGETS, &t);
	set_fs(old_fs);
	if (error)
		return error;

	__get_user(t.c_iflag, &it->c_iflag);
	t.c_iflag &= ~0100000; /* DOSMODE */

	__get_user(t.c_oflag, &it->c_oflag);

	__get_user(t.c_cflag, &it->c_cflag);
	if (t.c_cflag & 0100000) /* CRTSFL - SCO only? */
		t.c_cflag |= CRTSCTS;
	t.c_cflag &= ~0170000; /* LOBLK|CTSFLOW|RTSFLOW|CRTSFL */

	lflag = t.c_lflag;
	t.c_lflag &= ~0100777;
	__get_user(r, &it->c_lflag);
	t.c_lflag |= r;
	if ((t.c_lflag & 0100000))
		SYS(ioctl)(fd, TIOCEXCL, 0);
	else
		SYS(ioctl)(fd, TIOCNXCL, 0);
	t.c_lflag &= ~0100000;
	t.c_lflag |= (t.c_lflag & 0000400) << 7; /* Move IEXTEN */
	t.c_lflag &= ~0000400;
	t.c_lflag |= (t.c_lflag & 0001000) >> 1; /* Move TOSTOP */
	t.c_lflag &= ~0001000;
	t.c_lflag |= (lflag & 0001000); /* Restore ECHOCTL */

	copy_from_user(svr4_cc, &it->c_cc, SVR4_NCCS);
	t.c_cc[0] = svr4_cc[0];
	t.c_cc[1] = svr4_cc[1];
	t.c_cc[2] = svr4_cc[2];
	t.c_cc[3] = svr4_cc[3];
	t.c_cc[7] = svr4_cc[7];
	t.c_cc[8] = svr4_cc[8];
	t.c_cc[9] = svr4_cc[9];
	t.c_cc[10] = svr4_cc[10];
	t.c_cc[12] = svr4_cc[12];
	t.c_cc[13] = svr4_cc[13];
	t.c_cc[14] = svr4_cc[14];
	t.c_cc[15] = svr4_cc[15];
	t.c_cc[16] = svr4_cc[16];
	if (t.c_lflag & ICANON) {
		t.c_cc[4] = svr4_cc[4];
		t.c_cc[11] = svr4_cc[5];
	} else {
		t.c_cc[5] = svr4_cc[5];
		t.c_cc[6] = svr4_cc[4];
		t.c_cc[11] = svr4_cc[6];
	}

	set_fs(get_ds());
	error = SYS(ioctl)(fd, op, &t);
	set_fs(old_fs);

	return error;
}

static int linux_to_svr4_termios(int fd, int op, struct svr4_termios *it)
{
	struct termios t;
	char svr4_cc[SVR4_NCCS];
	mm_segment_t old_fs;
	int error;

	error = verify_area(VERIFY_WRITE, it, sizeof(struct svr4_termios));
	if (error)
		return error;

	old_fs = get_fs();
	set_fs(get_ds());
	error = SYS(ioctl)(fd, op, &t);
	set_fs(old_fs);
	if (error)
		return error;

	put_user(t.c_iflag & 0017777, &it->c_iflag);

	put_user(t.c_oflag & 0177777, &it->c_oflag);

	if (t.c_cflag & CRTSCTS)
		t.c_cflag |= 0100000; /* CRTSFL - SCO only? */
	put_user(t.c_cflag & 0177777, &it->c_cflag);

	t.c_lflag &= ~0001000;
	t.c_lflag |= (t.c_lflag & 0000400) << 1;
	t.c_lflag &= ~0000400;
	t.c_lflag |= (t.c_lflag & 0100000) >> 7;
	t.c_lflag &= ~0100000;
	put_user(t.c_lflag & 0001777, &it->c_lflag);

	svr4_cc[0] = t.c_cc[0];
	svr4_cc[1] = t.c_cc[1];
	svr4_cc[2] = t.c_cc[2];
	svr4_cc[3] = t.c_cc[3];
	svr4_cc[6] = t.c_cc[16];
	svr4_cc[7] = t.c_cc[7];
	svr4_cc[8] = t.c_cc[8];
	svr4_cc[9] = t.c_cc[9];
	svr4_cc[10] = t.c_cc[10];
	svr4_cc[11] = t.c_cc[10];
	svr4_cc[12] = t.c_cc[12];
	svr4_cc[13] = t.c_cc[13];
	svr4_cc[14] = t.c_cc[14];
	svr4_cc[15] = t.c_cc[15];
	if (t.c_lflag & ICANON) {
		svr4_cc[4] = t.c_cc[4];
		svr4_cc[5] = t.c_cc[11];
	} else {
		svr4_cc[4] = t.c_cc[6];
		svr4_cc[5] = t.c_cc[5];
	}

	copy_to_user(&it->c_cc, svr4_cc, SVR4_NCCS);

	return error;
}


static int ibcs_ioctl_termios(int fd, unsigned int func, void *arg)
{
#ifdef CONFIG_ABI_IBCS_SCO
	switch(func) {
	case 1:	/* XCGETA */
		return linux_to_sco_termios(fd, TCGETS, arg);
	case 2: /* XCSETA */
		return sco_to_linux_termios(fd, TCSETS, arg);
	case 3: /* XCSETAW */
		return sco_to_linux_termios(fd, TCSETSW, arg);
	case 4: /* XCSETAF */
		return sco_to_linux_termios(fd, TCSETSF, arg);
	}
#endif
	printk(KERN_ERR "iBCS: SCO termios ioctl %d unsupported\n", func);
	return -EINVAL;
}

static int ibcs_ioctl_termio(int fd, unsigned int func, void *arg)
{
	switch(func) {
		case 1: /* TCGETA  (TIOC|1) */
			return linux_to_svr_termio(fd, arg);

		case 2: /* TCSETA  (TIOC|2) */
			return svr_to_linux_termio(fd, TCSETA, arg);
		case 3: /* TCSETAW (TIOC|3) */
			return svr_to_linux_termio(fd, TCSETAW, arg);
		case 4: /* TCSETAF (TIOC|4) */
			return svr_to_linux_termio(fd, TCSETAF, arg);

		case 5: /* TCSBRK  (TIOC|5) */
			return SYS(ioctl)(fd, TCSBRK, arg);
		case 6: /* TCXONC  (TIOC|6) */
			return SYS(ioctl)(fd, TCXONC, arg);
		case 7: /* TCFLSH  (TIOC|7) */
			return SYS(ioctl)(fd, TCFLSH, arg);

		/* This group appear in SVR4 but not SVR3 (SCO). */
		case 8: /* TIOCKBON */
		case 9: /* TIOCKBOF */
		case 10: /* KBENABLED */
			return -EINVAL;

		/* This set is used by SVR4 for termios ioctls. */
		case 13: /* TCGETS */
			return linux_to_svr4_termios(fd, TCGETS, arg);
		case 14: /* TCSETS */
			return svr4_to_linux_termios(fd, TCSETS, arg);
		case 15: /* TCSETSW */
			return svr4_to_linux_termios(fd, TCSETSW, arg);
		case 16: /* TCSETSF */
			return svr4_to_linux_termios(fd, TCSETSF, arg);

		/* These two are specific to ISC. */
		case 20: /* TCSETPGRP  (TIOC|20) set pgrp of tty */
			return SYS(ioctl)(fd, TIOCSPGRP, arg);
		case 21: /* TCGETPGRP  (TIOC|21) get pgrp of tty */
			return SYS(ioctl)(fd, TIOCGPGRP, arg);

		case  34: /* TCGETSC (TIOC|34) ioctl for scancodes */
			return 0x04; /* Translates scancode to ascii */
		case  35: /* TCSETSC (TIOC|35) ioctl for scancodes */
			return 0;

		case 103: /* TIOCSWINSZ (TIOC|103) */
			return SYS(ioctl)(fd, TIOCSWINSZ, arg);
		case 104: /* TIOCGWINSZ (TIOC|104) */
			return SYS(ioctl)(fd, TIOCGWINSZ, arg);

		case 118: /* TIOCSPGRP  (TIOC|118) set pgrp of tty */
			return SYS(ioctl)(fd, TIOCSPGRP, arg);
		case 119: /* TIOCGPGRP  (TIOC|119) get pgrp of tty */
			return SYS(ioctl)(fd, TIOCGPGRP, arg);

		case  32: /* TCDSET  (TIOC|32) */
		case  33: /* RTS_TOG (TIOC|33) 386 - "RTS" toggle define 8A1 protocol */

		case 120: /* TIOSETSAK  (TIOC|120) set SAK sequence for tty */
		case 121: /* TIOGETSAK  (TIOC|121) get SAK sequence for tty */
			printk(KERN_ERR "iBCS: termio ioctl %d unimplemented\n",
				func);
			return -EINVAL;
	}
	printk(KERN_ERR "iBCS: termio ioctl %d unsupported\n", func);
	return -EINVAL;
}

struct termiox {
	unsigned short x_hflag;
	unsigned short x_cflag;
	unsigned short x_rflag[5];
	unsigned short x_sflag;
};

#define RTSXOFF 0x0001
#define CTSXON	0x0002

int svr4_ioctl_termiox(int fd, unsigned int func, void *arg)
{
	struct termios t;
	struct termiox tx;
	mm_segment_t old_fs;
	int error;

	if (func < 1 || func > 4)
		return -EINVAL;

	error = verify_area(func == 1 ? VERIFY_WRITE : VERIFY_READ,
			arg, sizeof(struct termiox));
	if (error)
		return error;

	old_fs = get_fs();
	set_fs(get_ds());
	error = SYS(ioctl)(fd, TCGETS, &t);
	set_fs(old_fs);
	if (error)
		return error;

	if (func == 1) { /* TCGETX */
		memset(&tx, '\0', sizeof(struct termiox));
		if (t.c_cflag & CRTSCTS)
			tx.x_hflag = RTSXOFF|CTSXON;
		copy_to_user(arg, &tx, sizeof(struct termiox));
		return 0;
	}

	copy_from_user(&tx, arg, sizeof(struct termiox));
	if ((tx.x_hflag != 0 && tx.x_hflag != (RTSXOFF|CTSXON))
	|| tx.x_cflag || tx.x_rflag[0] || tx.x_rflag[1]
	|| tx.x_rflag[2] || tx.x_rflag[3] || tx.x_rflag[4]
	|| tx.x_sflag)
		return -EINVAL;

	if (tx.x_hflag)
		t.c_cflag |= CRTSCTS;
	else
		t.c_cflag &= (~CRTSCTS);

	old_fs = get_fs();
	set_fs(get_ds());
	switch (func) {
		case 2: /* TCSETX */
			error = SYS(ioctl)(fd, TCSETS, &t);
			break;
		case 3: /* TCSETXW */
			error = SYS(ioctl)(fd, TCSETSW, &t);
			break;
		case 4: /* TCSETXF */
			error = SYS(ioctl)(fd, TCSETSF, &t);
			break;
	}
	set_fs(old_fs);
	return error;
}
EXPORT_SYMBOL(svr4_ioctl_termiox);


int svr4_ioctl_stream(struct pt_regs *regs, int fd, unsigned int func, void *arg)
{
	struct file *filep;
	struct inode *ino;
	int error;

	filep = fget(fd);
	if (!filep)
		return -EBADF;
	ino = filep->f_dentry->d_inode;

	if (ino && !ino->i_sock
	&& MAJOR(ino->i_rdev) == SOCKSYS_MAJOR) {
		error = abi_socksys_fd_init(fd, 0, NULL, NULL);
		if (error < 0)
			return error;
		fput(filep);
		filep = fget(fd);
		if (!filep)
			return -EBADF;
		ino = filep->f_dentry->d_inode;
	}

	switch (func) {
		case 001: /* I_NREAD */
			error = verify_area(VERIFY_WRITE,
					arg, sizeof(unsigned long));
			if (error) {
				fput(filep);
				return error;
			}
#ifdef CONFIG_ABI_XTI
			if (ino->i_sock && MAJOR(ino->i_rdev) == SOCKSYS_MAJOR)
				timod_update_socket(fd, filep, regs);

			if (ino->i_sock
			&& Priv(filep) && Priv(filep)->pfirst) {
				put_user(Priv(filep)->pfirst->length,
					(unsigned long *)arg);
				fput(filep);
				return 1; /* at least 1... (FIXME) */
			}
#endif
			fput(filep);
			error = SYS(ioctl)(fd, TIOCINQ, arg);
			if (error == -EINVAL)
				return 0;
			if (error)
				return error;
			__get_user(error, (unsigned long *)arg);
			return (error == 0 ? 0 : 1);

		case 017: { /* I_PEEK */
#ifdef CONFIG_ABI_XTI
			struct strpeek buf;

			error = verify_area(VERIFY_WRITE,
					arg, sizeof(struct strpeek));
			if (error) {
				fput(filep);
				return error;
			}
			copy_from_user(&buf, arg, sizeof(buf));

			if (ino->i_sock && MAJOR(ino->i_rdev) == SOCKSYS_MAJOR)
				timod_update_socket(fd, filep, regs);

			if (ino->i_sock
			&& Priv(filep) && Priv(filep)->pfirst
			&& (!buf.flags || buf.flags == Priv(filep)->pfirst->pri)) {
				int l = buf.ctl.maxlen <= Priv(filep)->pfirst->length
					? buf.ctl.maxlen
					: Priv(filep)->pfirst->length;
				error = verify_area(VERIFY_WRITE,
						buf.ctl.buf, l);
				if (error) {
					fput(filep);
					return error;
				}
				copy_to_user(buf.ctl.buf,
					((char *)&Priv(filep)->pfirst->type)
					+ Priv(filep)->offset, l);
				put_user(l, &((struct strpeek *)arg)->ctl.len);
				if (buf.dat.maxlen >= 0)
					put_user(0, &((struct strpeek *)arg)->dat.len);
				put_user(Priv(filep)->pfirst->pri,
					&((struct strpeek *)arg)->flags);
				fput(filep);
				return 1;
			}
			/* FIXME: I think we should also be able to peek
			 * at data as well?
			 */
#endif
			fput(filep);
			return 0; /* Nothing to peek at. */
		}
	}

	fput(filep);

	switch (func) {
		case 010: { /* I_STR */
			/* Unpack the ioctl data and forward as a normal
			 * ioctl. Timeouts are not handled (yet?).
			 */
			struct strioctl {
				int cmd, timeout, len;
				char *data;
			} it;

			error = verify_area(VERIFY_READ,
					arg, sizeof(struct strioctl));
			if (error)
				return error;
			copy_from_user(&it, arg, sizeof(struct strioctl));
#ifdef CONFIG_ABI_TRACE
			if (ibcs_trace & TRACE_STREAMS)
				printk(KERN_DEBUG "iBCS: STREAMS I_STR ioctl(%d, 0x%08lx, 0x%08lx)\n",
					fd, (unsigned long)it.cmd, (unsigned long)it.data);
#endif
			switch (it.cmd >> 8) {
				case 'T':/* timod */
#ifdef CONFIG_ABI_XTI
					return timod_ioctl(regs,
						fd, it.cmd & 0xff,
						it.data, it.len,
						&((struct strioctl *)arg)->len);
#else
					return -EINVAL;
#endif
				default:
					return do_ioctl(regs, fd, it.cmd, it.data);
			}
		}

		case 002: { /* I_PUSH */
			char *tmp;

			/* Get the name anyway to validate it. */
			tmp = getname(arg);
			if (IS_ERR(tmp))
				return PTR_ERR(tmp);
#ifdef CONFIG_ABI_TRACE
			if (ibcs_trace & TRACE_STREAMS)
				printk(KERN_DEBUG "iBCS: %d STREAMS I_PUSH %s\n",
					fd, tmp);
#endif
			putname(tmp);
			return 0;
		}

		case 003: /* I_POP */
#ifdef CONFIG_ABI_TRACE
			if (ibcs_trace & TRACE_STREAMS)
				printk(KERN_DEBUG "iBCS: %d STREAMS I_POP\n", fd);
#endif
			return 0;

		case 005: /* I_FLUSH */
			return 0;

		case 013: { /* I_FIND */
			char *tmp;

			/* Get the name anyway to validate it. */
			tmp = getname(arg);
			if (IS_ERR(tmp))
				return PTR_ERR(tmp);
#ifdef CONFIG_ABI_TRACE
			if (ibcs_trace & TRACE_STREAMS)
				printk(KERN_DEBUG "iBCS: %d STREAMS I_FIND %s\n",
					fd, tmp);
#endif
#ifdef CONFIG_ABI_XTI
			if (!strcmp(tmp, "timod")) {
				putname(tmp);
				return 1;
			}
#endif
			putname(tmp);
			return 0;
		}

		/* FIXME: These are bogus. */
		case 011: /* I_SETSIG */
			return SYS(ioctl)(fd, FIOSETOWN, current->pid);
		case 012: /* I_GETSIG */
			return SYS(ioctl)(fd, FIOGETOWN, arg);

		case 020: /* I_FDINSERT */
#ifdef CONFIG_ABI_XTI
			return stream_fdinsert(regs, fd,
					(struct strfdinsert *)arg);
#else
			return -EINVAL;
#endif

		case 004: /* I_LOOK */
		case 006: /* I_SRDOPT */
		case 007: /* I_GRDOPT */
		case 014: /* I_LINK */
		case 015: /* I_UNLINK */
		case 021: /* I_SENDFD */
		case 022: /* I_RECVFD */
		case 023: /* I_SWROPT */
		case 040: /* I_SETCLTIME */
			return 0; /* Lie... */
		case 042: /* I_CANPUT */
			/* Arg is the priority band in question. We only
			 * support one priority band so arg must be 0.
			 * If the band is writable we should return 1, if
			 * the band is flow controlled we should return 0.
			 */
			if (arg)
				return -EINVAL;

			/* FIXME: How can we test if a write would block? */
			return 1;

		case 024: /* I_GWROPT */
		case 025: /* I_LIST */
		case 026: /* I_PLINK */
		case 027: /* I_PUNLINK */
		case 030: /* I_SETEV */
		case 031: /* I_GETEV */
		case 032: /* I_STREV */
		case 033: /* I_UNSTREV */
		case 034: /* I_FLUSHBAND */
		case 035: /* I_CKBAND */
		case 036: /* I_GETBAND */
		case 037: /* I_ATMARK */
		case 041: /* I_GETCLTIME */
			/* Unsupported - drop out. */
	}
	printk(KERN_ERR "iBCS: STREAMS ioctl 0%o unsupported\n", func);
	return -EINVAL;
}

EXPORT_SYMBOL(svr4_ioctl_stream);


#ifdef CONFIG_ABI_IBCS_SCO
static int sco_ioctl_tape(int fd, unsigned int func, unsigned long arg)
{
	mm_segment_t old_fs;
	int error;
	struct mtop mtop;

	mtop.mt_count = 1;

	switch (func & 0xff) {
		case 1:  /* MT_RESET */
			mtop.mt_op = MTRESET;
			break;

		case 2:  /* MT_RETEN */
			mtop.mt_op = MTRETEN;
			break;

		case 3:  /* MT_REWIND */
			mtop.mt_op = MTREW;
			break;

		case 4:  /* MT_ERASE */
		case 23:  /* HP_ERASE */
			mtop.mt_op = MTERASE;
			break;

		case 6:  /* MT_RFM */
			mtop.mt_op = MTFSF;
			break;

		case 7:  /* MT_WFM */
			mtop.mt_op = MTWEOF;
			break;

		case 8:  /* MT_LOAD */
			mtop.mt_op = MTLOAD;
			break;

		case 9:  /* MT_UNLOAD */
			mtop.mt_op = MTOFFL;
			break;

		case 19:  /* MT_RSM */
			mtop.mt_op = MTFSS;
			break;

		case 20:  /* MT_WSM */
			mtop.mt_op = MTWSM;
			break;

		case 21:  /* MT_EOD */
			mtop.mt_op = MTEOM;
			break;

		case 24:  /* MT_SETBLK */
			mtop.mt_op = MTSETBLK;
			mtop.mt_count = arg;
			break;

		case 25:  /* MT_LOCK */
			mtop.mt_op = MTLOCK;
			break;

		case 26:  /* MT_UNLOCK */
			mtop.mt_op = MTUNLOCK;
			break;


#if 0
/*  The following function codes are just copied from the SCO
    include file.
*/
		case 0:  /* MT_STATUS */
		case 5:  /* MT_AMOUNT */
		case 10:  /* MT_DSTATUS */
		case 11:  /* MT_FORMAT */
		case 12:  /* MT_GETHDR */
		case 13:  /* MT_PUTHDR */
		case 14:  /* MT_GETNEWBB */
		case 15:  /* MT_PUTNEWBB */
		case 16:  /* MT_GETVTBL */
		case 17:  /* MT_PUTVTBL */
		case 18:  /* MT_SERVO */
		case 22:  /* MT_FORMPART */
		case 38:  /* MT_SETANSI */
		case 64:  /* MT_REPORT */
#endif
		default:
			printk (KERN_ERR "iBCS: SCO tape ioctl func=%d arg=%x unsupported\n",
				func & 0xff, (int) arg);
			return -EINVAL;
	}

	old_fs = get_fs ();
	set_fs (get_ds ());
	error = SYS (ioctl) (fd, MTIOCTOP, &mtop);
	set_fs (old_fs);
	return error;
}
#endif

static int ibcs_ioctl_tape(int fd, unsigned int func, unsigned long arg)
{
	mm_segment_t old_fs;
	int error;
	struct mtop mtop;

	mtop.mt_count = 1;

	switch (func & 0xff) {
		case 1:		/* MT_RETEN */
			mtop.mt_op = MTRETEN;
			break;

		case 2:		/* MT_REWIND */
			mtop.mt_op = MTREW;
			break;

		case 3:		/* MT_ERASE */
			mtop.mt_op = MTERASE;
			break;

		case 4:		/* MT_WFM */
			mtop.mt_op = MTWEOF;
			break;

		case 5:		/* MT_RESET */
			mtop.mt_op = MTRESET;
			break;

		case 7:		/* T_SFF */
			mtop.mt_op = MTFSF;
			break;

		case 8:		/* T_SBF */
			mtop.mt_op = MTBSF;
			break;

		case 9:		/* T_LOAD */
			mtop.mt_op = MTLOAD;
			break;

		case 10:  /* MT_UNLOAD */
			mtop.mt_op = MTOFFL;
			break;

		case 15:	/* T_WRBLKLEN */
			mtop.mt_op = MTLOCK;
			mtop.mt_count = arg;
			break;

		case 16:	/* T_PREVMV */
			mtop.mt_op = MTLOCK;
			break;

		case 17:	/* T_ALLOMV */
			mtop.mt_op = MTUNLOCK;
			break;

		case 20:	/* T_EOD */
			mtop.mt_count = arg;
			mtop.mt_op = MTEOM;
			break;

		case 21:	/* T_SSFB */
			mtop.mt_count = arg;
			mtop.mt_op = MTBSFM;
			break;

		case 22:	/* T_SSFF */
			mtop.mt_count = arg;
			mtop.mt_op = MTFSFM;
			break;

		case 24:	/* T_STD */
			mtop.mt_count = arg;
			mtop.mt_op = MTSETDENSITY;
			break;

#if 0
		case 6:		/* T_STATUS */
		case 14:	/* T_RDBLKLEN */
		case 18:	/* T_SBB */
		case 19:	/* T_SFB */
		case 23:	/* T_STS */
#endif
		default:
			printk (KERN_ERR "iBCS: SYSV tape ioctl func=%d arg=%x unsupported\n",
				func & 0xff, (int) arg);
			return -EINVAL;
	}

	old_fs = get_fs ();
	set_fs (get_ds ());
	error = SYS (ioctl) (fd, MTIOCTOP, &mtop);
	set_fs (old_fs);
	return error;
}


int svr4_ioctl(struct pt_regs * regs)
{
	int fd;
	unsigned int ioctl_num;
	void *arg;

	fd = (int)get_syscall_parameter (regs, 0);
	ioctl_num = (unsigned int)get_syscall_parameter (regs, 1);
	arg = (void *)get_syscall_parameter (regs, 2);
	return do_ioctl(regs, fd, ioctl_num, arg);
}

EXPORT_SYMBOL(svr4_ioctl);
