/*
 * Copyright (c) 2001 Caldera Deutschland GmbH.
 * Copyright (c) 2001 Christoph Hellwig.
 * All rights reserved.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef _ASM_ABI_REG_H
#define _ASM_ABI_REG_H

#include <linux/ptrace.h>

#if _KSL > 24

#define _AX(x) x->ax
#define _BX(x) x->bx
#define _CX(x) x->cx
#define _DX(x) x->dx
#define _SI(x) x->si
#define _DI(x) x->di
#define _BP(x) x->bp
#define _SP(x) x->sp
#define _IP(x) x->ip
#define _DS(x) x->ds
#define _ES(x) x->es
#define _CS(x) x->cs
#define _SS(x) x->ss
#define _FS(x) x->fs
#define _GS(x) x->gs

#define _OAX(x) x->orig_ax
#define _FLG(x) x->flags

#else
#ifdef CONFIG_64BIT
#define _AX(x) x->rax
#define _BX(x) x->rbx
#define _CX(x) x->rcx
#define _DX(x) x->rdx
#define _SI(x) x->rsi
#define _DI(x) x->rdi
#define _BP(x) x->rbp
#define _SP(x) x->rsp
#define _IP(x) x->rip
#define _CS(x) x->cs
#define _SS(x) x->ss
#define _OAX(x) x->orig_rax
#define _FLG(x) x->eflags

#else

#define _AX(x) x->eax
#define _BX(x) x->ebx
#define _CX(x) x->ecx
#define _DX(x) x->edx
#define _SI(x) x->esi
#define _DI(x) x->edi
#define _BP(x) x->ebp
#define _SP(x) x->esp
#define _IP(x) x->eip
#define _DS(x) x->xds
#define _ES(x) x->xes
#define _CS(x) x->xcs
#define _SS(x) x->xss
#define _FS(x) x->xfs
#define _GS(x) x->xgs
#define _OAX(x) x->orig_eax
#define _FLG(x) x->eflags
#endif /* 64BIT */

#endif /* KSL */

#endif /* _ASM_ABI_REG_H */
