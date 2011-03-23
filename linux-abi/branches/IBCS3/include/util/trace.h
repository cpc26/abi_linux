/*
 * Copyright (c) 2001 Christoph Hellwig.
 * All rights resered.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef _ABI_TRACE_H_
#define _ABI_TRACE_H_

//#ident "%W% %G%"

/*
 * Linux-ABI tracing helpers.
 */
#include <linux/types.h>
//#include <asm/unistd.h>
#include "i386_std.h"
extern asmlinkage int sys_call(const int sys_code,...);
#define SYS(name,a...) sys_call(__NR_##name, ## a)
/*-----------------------------------------------------*/
extern unsigned int abi_map(unsigned int,int);
#ifndef MAP_32BIT
#define MAP_32BIT 0
#endif
/*-----------------------------------------------------*/
#define Y2K_SHIFT 86400			// Secoinds in a day = 60*60*24
#define Y2K_FEB29 11015*86400		// Number of seconds since 01/01/70
#define ABI_Y2K_BUG 0x00000100
#define y2k_send(x) if (current->personality & ABI_Y2K_BUG) if (x > Y2K_FEB29) x -= Y2K_SHIFT
#define y2k_recv(x) if (current->personality & ABI_Y2K_BUG) if (x > Y2K_FEB29 - Y2K_SHIFT) x += Y2K_SHIFT

/*
 * Tracing flags.
 */
enum {
	ABI_TRACE_API =		0x00000001, /* all call/return values	*/
	ABI_TRACE_IOCTL =	0x00000002, /* all ioctl calls		*/
	ABI_TRACE_IOCTL_F =	0x00000004, /* ioctl calls that fail	*/
	ABI_TRACE_SIGNAL =	0x00000008, /* all signal calls		*/
	ABI_TRACE_SIGNAL_F =	0x00000010, /* signal calls that fail	*/
	ABI_TRACE_SOCKSYS =	0x00000020, /* socksys and spx devices	*/
	ABI_TRACE_STREAMS =	0x00000040, /* STREAMS faking		*/
	ABI_TRACE_UNIMPL =	0x00000080, /* unimplemened functions	*/
};
extern u_int	abi_trace_flg;


/*
 * Check if a syscall needs tracing.
 */
#define abi_traced(res)		(abi_trace_flg & (res))

/*
 * Unconditinal trace.
 */
#define __abi_trace(fmt...)						\
do {									\
	printk(KERN_DEBUG "[%s:%d]: ", current->comm, current->pid);	\
	printk(fmt);							\
} while(0)

/*
 * Trace depending on reason.
 */
#define abi_trace(res, fmt...)						\
do {									\
	if (abi_traced(res))						\
		__abi_trace(fmt);					\
} while(0)

/* prototype for ./abi/util/plist.h */
extern void plist(char *, char *, int *);
extern int pdump(char *, int);

#endif /* _ABI_TRACE_H_ */
