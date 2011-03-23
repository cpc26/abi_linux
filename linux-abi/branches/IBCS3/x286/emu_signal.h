/*
 *	emu_signal.h -- Header for emu_signal.c
 *
 *	Copyright (C) 1998 Hulcote Electronics (Europe) Ltd.
 *			   David Bruce (David@Hulcote.com)
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef _EMU_SIGNAL_H_
#define _EMU_SIGNAL_H_

extern void sig_sigf(int , struct sigcontext );
extern void init_sigs(void);
extern int emu_i_signal(struct sigcontext *);
extern int emu_i_kill(struct sigcontext *);
extern void do_sig_pending(int , volatile struct sigcontext *);
extern void xigret(void);

struct xigaction {
	void (*handler)(int);
	unsigned long mask;
	unsigned long flags;
	void (*restorer)(void);
};

#endif /* _EMU_SIGNAL_H_ */
