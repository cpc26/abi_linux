/*
 *  linux/abi/abi_common/ioctl.c
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

#include <asm/uaccess.h>

#include <abi/abi.h>
#include <abi/bsd.h>
#include <abi/stream.h>
#include <abi/tli.h>
#include <abi/abi4.h>
#ifdef CONFIG_ABI_TRACE
#include <abi/trace.h>
#endif


int abi_ioctl_socksys(int fd, unsigned int func, void *arg);


static inline char *fix(int n) {
	static char char_class[4];
	
	 char_class[0] = n & 0xFF0000 ? (char)((n >> 16) & 0xFF) : '.';
	 char_class[1] = n & 0x00FF00 ? (char)((n >>  8) & 0xFF) : '.';
	 char_class[2] = n & 0x0000FF ? (char)((n      ) & 0xFF) : '.';
	 char_class[3] = 0;

	return char_class;
}

#define BSD_NCCS 20
struct bsd_termios {
	unsigned long	c_iflag;
	unsigned long	c_oflag;
	unsigned long	c_cflag;
	unsigned long	c_lflag;
	unsigned char	c_cc[BSD_NCCS];
	long		c_ispeed;
	long		c_ospeed;
};
static unsigned long speed_map[] = {
	0, 50, 75, 110, 134, 150, 200, 300, 600, 1200, 1800, 2400,
	4800, 9600, 19200, 38400
};

static unsigned long
bsd_to_linux_speed(unsigned long s)
{
	unsigned int i;

#ifdef B57600
	if (s == 57600)
		return B57600;
#endif
#ifdef B115200
	if (s == 115200)
		return B115200;
#endif
	
	for (i=0; i<sizeof(speed_map)/sizeof(speed_map[0]); i++)
		if (s <= speed_map[i])
			return i;
	return B38400;
}

static unsigned long
linux_to_bsd_speed(unsigned long s)
{
#ifdef B57600
	if (s == B57600)
		return 57600;
#endif
#ifdef B115200
	if (s == B115200)
		return 115200;
#endif
	return speed_map[s];
}




static int
bsd_to_linux_termios(int fd, int op, struct bsd_termios *it)
{
	struct termios t;
	mm_segment_t old_fs;
	unsigned long temp;
	char bsd_cc[BSD_NCCS];
	int error;

	error = verify_area(VERIFY_READ, it, sizeof(struct bsd_termios));
	if (error)
		return error;

	old_fs = get_fs();
	set_fs(get_ds());
	error = SYS(ioctl)(fd, TCGETS, &t);
	set_fs(old_fs);
	if (error)
		return error;

	__get_user(t.c_iflag, &it->c_iflag);
	t.c_iflag = (t.c_iflag & ~0xc00)
			| ((t.c_iflag & 0x400) << 1)
			| ((t.c_iflag & 0x800) >> 1);

	get_user(temp, &it->c_oflag);
	t.c_oflag = (t.c_oflag & ~0x1805)
			| (temp & 9)
			| ((temp & 2) << 1)
			| ((temp & 4) << 10)
			| ((temp & 4) << 9);

	get_user(temp, &it->c_cflag);
	t.c_cflag = (t.c_cflag & ~0xfff)
			| ((temp & 0xff00) >> 4);
	if (t.c_cflag & 0x30000)
		t.c_cflag |= 020000000000;
	t.c_cflag |= bsd_to_linux_speed(({long s; get_user(s, &it->c_ospeed); s;}))
		| (bsd_to_linux_speed(({long s; get_user(s, &it->c_ispeed); s;})) << 16);

	get_user(temp, &it->c_lflag);
	t.c_lflag = (t.c_lflag & ~0157663)
			| ((temp & 1) << 12)
			| ((temp & 0x46) << 3)
			| ((temp & 0x420) << 5)
			| ((temp & 0x180) >> 7)
			| ((temp & 0x400000) >> 14)
			| ((temp & 0x2800000) >> 11)
			| ((temp & 0x80000000) >> 24);

	copy_from_user(bsd_cc, &it->c_cc, BSD_NCCS);
	t.c_cc[VEOF] = bsd_cc[0];
	t.c_cc[VEOL] = bsd_cc[1];
	t.c_cc[VEOL2] = bsd_cc[2];
	t.c_cc[VERASE] = bsd_cc[3];
	t.c_cc[VWERASE] = bsd_cc[4];
	t.c_cc[VKILL] = bsd_cc[5];
	t.c_cc[VREPRINT] = bsd_cc[6];
	t.c_cc[VSWTC] = bsd_cc[7];
	t.c_cc[VINTR] = bsd_cc[8];
	t.c_cc[VQUIT] = bsd_cc[9];
	t.c_cc[VSUSP] = bsd_cc[10];
/*	t.c_cc[VDSUSP] = bsd_cc[11];*/
	t.c_cc[VSTART] = bsd_cc[12];
	t.c_cc[VSTOP] = bsd_cc[13];
	t.c_cc[VLNEXT] = bsd_cc[14];
	t.c_cc[VDISCARD] = bsd_cc[15];
	t.c_cc[VMIN] = bsd_cc[16];
	t.c_cc[VTIME] = bsd_cc[17];
/*	t.c_cc[VSTATUS] = bsd_cc[18];*/

	set_fs(get_ds());
	error = SYS(ioctl)(fd, op, &t);
	set_fs(old_fs);

	return error;
}


static int
linux_to_bsd_termios(int fd, int op, struct bsd_termios *it)
{
	struct termios t;
	char bsd_cc[BSD_NCCS];
	mm_segment_t old_fs;
	int error;

	error = verify_area(VERIFY_WRITE, it, sizeof(struct bsd_termios));
	if (error)
		return error;

	old_fs = get_fs();
	set_fs(get_ds());
	error = SYS(ioctl)(fd, op, &t);
	set_fs(old_fs);
	if (error)
		return error;

	put_user((t.c_iflag & 0777)
			| ((t.c_iflag & 02000) >> 1)
			| ((t.c_iflag & 010000) >> 2)
			| ((t.c_iflag & 020000) >> 4),
		&it->c_iflag);

	put_user((t.c_oflag & 1)
			| ((t.c_oflag & 04) >> 1)
			| ((t.c_oflag & 014000) == 014000 ? 4 : 0),
		&it->c_oflag);

	put_user((t.c_cflag & ~020000007777)
			| ((t.c_cflag & 0xff0) << 4)
			| ((t.c_cflag & 020000000000) ? 0x30000 : 0),
		&it->c_cflag);

	put_user(linux_to_bsd_speed(t.c_cflag & CBAUD), &it->c_ospeed);
	if ((t.c_cflag & CIBAUD) != 0)
		put_user(linux_to_bsd_speed((t.c_cflag & CIBAUD) >> 16),
			&it->c_ispeed);
	else
		put_user(linux_to_bsd_speed(t.c_cflag & CBAUD),
			&it->c_ispeed);

	put_user((t.c_lflag & 07777626010)
			| ((t.c_lflag & 03) << 7)
			| ((t.c_lflag & 01160) >> 3)
			| ((t.c_lflag & 0400) << 14)
			| ((t.c_lflag & 02000) >> 4)
			| ((t.c_lflag & 04000) >> 11)
			| ((t.c_lflag & 010000) << 11)
			| ((t.c_lflag & 040000) << 15)
			| ((t.c_lflag & 0100000) >> 5),
		&it->c_lflag);

	bsd_cc[0] = t.c_cc[VEOF];
	bsd_cc[1] = t.c_cc[VEOL];
	bsd_cc[2] = t.c_cc[VEOL2];
	bsd_cc[3] = t.c_cc[VERASE];
	bsd_cc[4] = t.c_cc[VWERASE];
	bsd_cc[5] = t.c_cc[VKILL];
	bsd_cc[6] = t.c_cc[VREPRINT];
	bsd_cc[7] = t.c_cc[VSWTC];
	bsd_cc[8] = t.c_cc[VINTR];
	bsd_cc[9] = t.c_cc[VQUIT];
	bsd_cc[10] = t.c_cc[VSUSP];
	bsd_cc[11] = t.c_cc[VSUSP];
	bsd_cc[12] = t.c_cc[VSTART];
	bsd_cc[13] = t.c_cc[VSTOP];
	bsd_cc[14] = t.c_cc[VLNEXT];
	bsd_cc[15] = t.c_cc[VDISCARD];
	bsd_cc[16] = t.c_cc[VMIN];
	bsd_cc[17] = t.c_cc[VTIME];
	bsd_cc[18] = 0; /* t.c_cc[VSTATUS]; */
	bsd_cc[19] = 0;

	copy_to_user(&it->c_cc, bsd_cc, BSD_NCCS);

	return error;
}




struct v7_sgttyb {
	unsigned char	sg_ispeed;
	unsigned char	sg_ospeed;
	unsigned char	sg_erase;
	unsigned char	sg_kill;
	int	sg_flags;
};

struct v7_tchars {
	char	t_intrc;
	char	t_quitc;
	char	t_startc;
	char	t_stopc;
	char	t_eofc;
	char	t_brkc;
};

struct v7_ltchars {
	char	t_suspc;
	char	t_dsuspc;
	char	t_rprntc;
	char	t_flushc;
	char	t_werasc;
	char	t_lnextc;
};


int bsd_ioctl_termios(int fd, unsigned int func, void *arg)
{
	switch (func & 0xff) {
		case 0:	 {				/* TIOCGETD */
			unsigned long ldisc;
			mm_segment_t old_fs;
			int error;

			error = verify_area(VERIFY_WRITE, arg,
					sizeof(unsigned short));
			if (error)
				return error;

			old_fs = get_fs();
			set_fs(get_ds());
			error = SYS(ioctl)(fd, TIOCGETD, &ldisc);
			set_fs(old_fs);
			if (!error)
				put_user(ldisc, (unsigned short *)arg);
			return error;
		}
		case 1: {				/* TIOCSETD */
			unsigned long ldisc;
			mm_segment_t old_fs;
			int error;

			error = verify_area(VERIFY_READ, arg,
					sizeof(unsigned short));
			if (error)
				return error;

			get_user(ldisc, (unsigned short *)arg);
			old_fs = get_fs();
			set_fs(get_ds());
			error = SYS(ioctl)(fd, TIOCSETD, &ldisc);
			set_fs(old_fs);
			return error;
		}

		case 2: {				/* TIOCHPCL */
			int error;
			mm_segment_t old_fs;
			struct termios t;

			old_fs = get_fs();
			set_fs(get_ds());
			error = SYS(ioctl)(fd, TCGETS, &t);
			set_fs(old_fs);
			if (error)
				return error;

			if (arg)
				t.c_cflag |= HUPCL;
			else
				t.c_cflag &= ~HUPCL;

			old_fs = get_fs();
			set_fs(get_ds());
			error = SYS(ioctl)(fd, TCSETS, &t);
			set_fs(old_fs);
			return error;
		}

		case 8: {				/* TIOCGETP */
			int error;
			mm_segment_t old_fs;
			struct termios t;
			struct v7_sgttyb sg;

			error = verify_area(VERIFY_WRITE, arg, sizeof(sg));
			if (error)
				return error;

			old_fs = get_fs();
			set_fs(get_ds());
			error = SYS(ioctl)(fd, TCGETS, &t);
			set_fs(old_fs);
			if (error)
				return error;

			sg.sg_ispeed = sg.sg_ospeed = 0;
			sg.sg_erase = t.c_cc[VERASE];
			sg.sg_kill = t.c_cc[VKILL];
			sg.sg_flags =
				/* Old - became TANDEM instead.
				 * ((t.c_cflag & HUPCL) >> 10)
				 * |
				 */
/* O_ODDP */			((t.c_cflag & PARODD) >> 3)
/* O_EVENP */			| ((t.c_cflag & PARENB) >> 1)
/* LITOUT */			| ((t.c_cflag & OPOST) ? 0 : 0x200000)
/* O_CRMOD */			| ((t.c_oflag & ONLCR) << 2)
/* O_NL1|O_VTDELAY */		| (t.c_oflag & (NL1|VTDLY))
/* O_TBDELAY */			| ((t.c_oflag & TABDLY) ? 02000 : 0)
/* O_CRDELAY */			| ((t.c_oflag & CRDLY) << 3)
/* O_BSDELAY */			| ((t.c_oflag & BSDLY) << 2)
/* O_ECHO|O_LCASE */		| (t.c_lflag & (XCASE|ECHO))
				| ((t.c_lflag & ICANON)
/* O_CBREAK or O_RAW */		? 0 : ((t.c_lflag & ISIG) ? 0x02 : 0x20))
				/* Incomplete... */
				;

			copy_to_user(arg, &sg, sizeof(sg));
			return 0;
		}

		case 9:					/* TIOCSETP */
		case 10: {				/* TIOCSETN */
			int error;
			mm_segment_t old_fs;
			struct termios t;
			struct v7_sgttyb sg;

			error = verify_area(VERIFY_READ, arg, sizeof(sg));
			if (error)
				return error;
			copy_from_user(&sg, arg, sizeof(sg));

			old_fs = get_fs();
			set_fs(get_ds());
			error = SYS(ioctl)(fd, TCGETS, &t);
			set_fs(old_fs);
			if (error)
				return error;

			t.c_cc[VERASE] = sg.sg_erase;
			t.c_cc[VKILL] = sg.sg_kill;
			t.c_iflag = ICRNL | IXON;
			t.c_oflag = 0;
			t.c_lflag = ISIG | ICANON;
			if (sg.sg_flags & 0x02)		/* O_CBREAK */
				t.c_lflag &= (~ICANON);
			if (sg.sg_flags & 0x08)		/* O_ECHO */
				t.c_lflag |= ECHO|ECHOE|ECHOK|ECHOCTL|ECHOKE|IEXTEN;
			if (sg.sg_flags & 0x10)	/* O_CRMOD */
				t.c_oflag |= OPOST|ONLCR;
			if (sg.sg_flags & 0x20) {	/* O_RAW */
				t.c_iflag = 0;
				t.c_lflag &= ~(ISIG|ICANON);
			}
			if (sg.sg_flags & 0x200000)	/* LITOUT */
				t.c_oflag &= (~OPOST);
			if (!(t.c_lflag & ICANON)) {
				t.c_cc[VMIN] = 1;
				t.c_cc[VTIME] = 0;
			}

			old_fs = get_fs();
			set_fs(get_ds());
			error = SYS(ioctl)(fd, TCSETS, &t);
			set_fs(old_fs);
			return error;
		}

		case 17: {				/* TIOCSETC */
			int error;
			mm_segment_t old_fs;
			struct termios t;
			struct v7_tchars tc;

			error = verify_area(VERIFY_READ, arg, sizeof(tc));
			if (error)
				return error;
			copy_from_user(&tc, arg, sizeof(tc));

			old_fs = get_fs();
			set_fs(get_ds());
			error = SYS(ioctl)(fd, TCGETS, &t);
			set_fs(old_fs);
			if (error)
				return error;

			t.c_cc[VINTR] = tc.t_intrc;
			t.c_cc[VQUIT] = tc.t_quitc;
			t.c_cc[VSTART] = tc.t_startc;
			t.c_cc[VSTOP] = tc.t_stopc;
			t.c_cc[VEOF] = tc.t_eofc;
			t.c_cc[VEOL2] = tc.t_brkc;

			old_fs = get_fs();
			set_fs(get_ds());
			error = SYS(ioctl)(fd, TCSETS, &t);
			set_fs(old_fs);
			return error;
		}

		case 18: {				/* TIOCGETC */
			int error;
			mm_segment_t old_fs;
			struct termios t;
			struct v7_tchars tc;

			error = verify_area(VERIFY_WRITE, arg, sizeof(tc));
			if (error)
				return error;

			old_fs = get_fs();
			set_fs(get_ds());
			error = SYS(ioctl)(fd, TCGETS, &t);
			set_fs(old_fs);
			if (error)
				return error;

			tc.t_intrc = t.c_cc[VINTR];
			tc.t_quitc = t.c_cc[VQUIT];
			tc.t_startc = t.c_cc[VSTART];
			tc.t_stopc = t.c_cc[VSTOP];
			tc.t_eofc = t.c_cc[VEOF];
			tc.t_brkc = t.c_cc[VEOL2];

			copy_to_user(arg, &tc, sizeof(tc));
			return 0;
		}

		case 116: {				/* TIOCGLTC */
			int error;
			mm_segment_t old_fs;
			struct termios t;
			struct v7_ltchars tc;

			error = verify_area(VERIFY_WRITE, arg, sizeof(tc));
			if (error)
				return error;

			old_fs = get_fs();
			set_fs(get_ds());
			error = SYS(ioctl)(fd, TCGETS, &t);
			set_fs(old_fs);
			if (error)
				return error;

			tc.t_suspc = t.c_cc[VSUSP];
			tc.t_dsuspc = t.c_cc[VSUSP];
			tc.t_rprntc = t.c_cc[VREPRINT];
			tc.t_flushc = t.c_cc[VEOL2];
			tc.t_werasc = t.c_cc[VWERASE];
			tc.t_lnextc = t.c_cc[VLNEXT];

			copy_to_user(arg, &tc, sizeof(tc));
			return 0;
		}

		case 117: {				/* TIOCSLTC */
			int error;
			mm_segment_t old_fs;
			struct termios t;
			struct v7_ltchars tc;

			error = verify_area(VERIFY_READ, arg, sizeof(tc));
			if (error)
				return error;
			copy_from_user(&tc, arg, sizeof(tc));

			old_fs = get_fs();
			set_fs(get_ds());
			error = SYS(ioctl)(fd, TCGETS, &t);
			set_fs(old_fs);
			if (error)
				return error;

			t.c_cc[VSUSP] = tc.t_suspc;
			t.c_cc[VEOL2] = tc.t_dsuspc;
			t.c_cc[VREPRINT] = tc.t_rprntc;
			t.c_cc[VEOL2] = tc.t_flushc;
			t.c_cc[VWERASE] = tc.t_werasc;
			t.c_cc[VLNEXT] = tc.t_lnextc;

			old_fs = get_fs();
			set_fs(get_ds());
			error = SYS(ioctl)(fd, TCSETS, &t);
			set_fs(old_fs);
			return error;
		}

		case 13:				/* TIOEXCL */
			return SYS(ioctl)(fd, TIOCEXCL, arg);

		case 14:				/* TIOCNXCL */
			return SYS(ioctl)(fd, TIOCNXCL, arg);

		case 16:				/* TIOCFLUSH */
			return SYS(ioctl)(fd, TCFLSH, arg);

		/* ISC (maybe SVR4 in general?) has some extensions over
		 * the sgtty stuff. So do later BSDs. Needless to say they
		 * both have different extensions.
		 */
		case 20: /* TCSETPGRP  (TIOC|20) set pgrp of tty */
			return bsd_to_linux_termios(fd, TCSETS, arg);

		case 21: /* TCGETPGRP  (TIOC|21) get pgrp of tty */
			return bsd_to_linux_termios(fd, TCSETSW, arg);

		case 19:				/* TIOCGETA */
			return linux_to_bsd_termios(fd, TCGETS, arg);

		case 22:				/* TIOCSETAF */
 			return bsd_to_linux_termios(fd, TCSETSF, arg);

		case 26:				/* TIOCGETD */
			return SYS(ioctl)(fd, TIOCGETD, arg);

		case 27:				/* TIOCSETD */
			return SYS(ioctl)(fd, TIOCSETD, arg);

		case 97:				/* TIOCSCTTY */
			return SYS(ioctl)(fd, TIOCSCTTY, arg);

		case 103:				/* TIOCSWINSZ */
			return SYS(ioctl)(fd, TIOCSWINSZ, arg);

		case 104:				/* TIOCGWINSZ */
			return SYS(ioctl)(fd, TIOCGWINSZ, arg);

		case 113:				/* TIOCNOTTY */
			return SYS(ioctl)(fd, TIOCNOTTY, arg);

		case 118:	 			/* TIOCSPGRP */
			return SYS(ioctl)(fd, TIOCSPGRP, arg);

		case 119:				/* TIOCGPGRP */
			return SYS(ioctl)(fd, TIOCGPGRP, arg);

		case 123:				/* TIOCSBRK */
			return SYS(ioctl)(fd, TCSBRK, arg);

		case 124:				/* TIOCLGET */
		case 125:				/* TIOCLSET */
			return 0;


		case 3:					/* TIOCMODG */
		case 4:					/* TIOCMODS */
		case 94:				/* TIOCDRAIN */
		case 95:				/* TIOCSIG */
		case 96:				/* TIOCEXT */
		case 98:				/* TIOCCONS */
		case 102:				/* TIOCUCNTL */
		case 105:				/* TIOCREMOTE */
		case 106:				/* TIOCMGET */
		case 107:				/* TIOCMBIC */
		case 108:				/* TIOCMBIS */
		case 109:				/* TIOCMSET */
		case 110:				/* TIOCSTART */
		case 111:				/* TIOCSTOP */
		case 112:				/* TIOCPKT */
		case 114:				/* TIOCSTI */
		case 115:				/* TIOCOUTQ */
		case 120:				/* TIOCCDTR */
		case 121:				/* TIOCSDTR */
		case 122:				/* TIOCCBRK */

	}

	printk(KERN_ERR "BSD/V7: terminal ioctl 0x%08lx unsupported\n",
		(unsigned long)func);
	return -EINVAL;
}


EXPORT_SYMBOL(bsd_ioctl_termios);


int abi_ioctl_console(int fd, unsigned int func, void *arg) {
	switch(func) {
		case 0x6301: /* CONS_CURRENT: Get display adapter type */
		case 0x6302: /* CONS_GET: Get display mode setting */
			/* Always error so the application never tries
			 * anything overly fancy on the console.
			 */
			return -EINVAL;
		case 0x4304: /* _TTYDEVTYPE */
			/* If on console then 1, if pseudo tty then 2 */
			return 2;
	}
	printk(KERN_ERR "iBCS: console ioctl %d unsupported\n", func);
	return -EINVAL;
}

EXPORT_SYMBOL(abi_ioctl_console);

int abi_ioctl_video(int fd, unsigned int func, void *arg) {
	switch(func) {
		case 1: /* MAP_CLASS */
			/* Get video memory map & IO privilege */
		/* This doesn't agree with my SCO 3.2.4 ???? */
		case 4: /* C_IOC */
			/* see /etc/conf/pack.d/cn/class.h on any SCO unix box :-) */
	}
	printk(KERN_ERR "iBCS: video ioctl %d unsupported\n", func);
	return -EINVAL;
}

EXPORT_SYMBOL(abi_ioctl_video);
