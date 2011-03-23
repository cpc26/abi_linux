/*
 *   abi/svr4_common/vtkd.c
 *
 * This provides internal emulation support for the SCO <sys/vtkd.h> on
 * the multiscreen console. More or less, this involves translating the
 * input ioctl()'s into a similar Linux ioctl()'s.
 *
 * Not emulated SCO multiscreen functions:
 *   None.
 *
 * Not emulated SCO keyboard functions:
 *   KIOCDOSMODE		set DOSMODE
 *   KIOCNONDOSMODE		unset DOSMODE
 *   KDDISPINFO			get display start and size
 *   KDGKBSTATE			get state of keyboard shift keys
 *
 * Written by Scott Michel, scottm@intime.com
 * (c) 1994 Scott Michel as part of the Linux iBCS-2 emulator project.
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
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/vt.h>
#include <linux/kd.h>

#include <abi/abi.h>
#include <abi/trace.h>

static struct {
    int in_ioctl;					/* only lower 8 bits */
    int out_ioctl;					/* Linux version */
} trantab[] = {
#ifdef KDDISPTYPE
    { 1,  KDDISPTYPE   },
#endif
    { 2,  KDMAPDISP    },
    { 3,  KDUNMAPDISP  },
    { 6,  KDGKBMODE    },
    { 7,  KDSKBMODE    },
    { 8,  KDMKTONE     },
    { 9,  KDGETMODE    },
    { 10, KDSETMODE    },
    { 11, KDADDIO      },
    { 12, KDDELIO      },
    { 60, KDENABIO     },
    { 61, KDDISABIO    },
#ifdef KIOCINFO
    { 62, KIOCINFO     },
#endif
    { 63, KIOCSOUND    },
    { 64, KDGKBTYPE    },
    { 65, KDGETLED     },
    { 66, KDSETLED     },
};

/*--------------------------------------------------------------------------
 * ibcs_ioctl_vtkd()
 *------------------------------------------------------------------------*/

int
ibcs_ioctl_vtkd(int fd, int todo, void *arg)
{
    int gen = (todo >> 8) & 0xff, spec = todo & 0xff;
    int newf;

    if (gen == 'v') {
	/* Could make this translation process table based, but, why
	   waste the valuable kernel space? */

	newf = (spec == 1 ? VT_OPENQRY :
		(spec == 2 ? VT_SETMODE :
		 (spec == 3 ? VT_GETMODE :
		  (spec == 4 ? VT_RELDISP :
		   (spec == 5 ? VT_ACTIVATE : -1)))));
	if (newf != -1)
	    return SYS(ioctl)(fd, newf, arg);
    } else if (gen == 'K') {
	register unsigned int i;

	for (i = 0; i < sizeof(trantab) / sizeof(trantab[0]); ++i) {
	    if (spec == trantab[i].in_ioctl)
		return SYS(ioctl)(fd, trantab[i].out_ioctl, arg);
	}
    }

    printk(KERN_ERR "iBCS: vtkd ioctl 0x%02x unsupported\n", todo);
    return -EINVAL;
}
