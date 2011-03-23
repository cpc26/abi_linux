/*
 *  linux/abi/svr4_common/timod.c
 *
 *  Copyright 1995, 1996  Mike Jagdis (jaggy@purplet.demon.co.uk)
 *
 * $Id$
 * $Source$
 */


#include <linux/config.h>
#ifdef CONFIG_ABI_XTI
#define __NO_VERSION__
#include <linux/module.h>
#include <linux/version.h>

#include <asm/uaccess.h>

#include <linux/types.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/ptrace.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/malloc.h>
#include <linux/mm.h>
#include <linux/fcntl.h>
#include <linux/socket.h>
#include <linux/in.h>
#include <linux/un.h>
#include <linux/file.h>
#include <linux/poll.h>

#include <abi/abi.h>
#include <abi/stream.h>
#include <abi/tli.h>

#ifdef CONFIG_ABI_TRACE
#include <abi/trace.h>
#endif

#ifdef __sparc__
/* ibcs2 wants to access the stack pointer, let's give it an alias
 * for the sparc
 */
#define esp u_regs[UREG_FP]
#endif

#ifdef CONFIG_ABI_TRACE
static char *
xti_prim(int n)
{
	char *tab[] = {
		"T_CONN_REQ", "T_CONN_RES", "T_DISCON_REQ", "T_DATA_REQ",
		"T_EXDATA_REQ", "T_INFO_REQ", "T_BIND_REQ", "T_UNBIND_REQ",
		"T_UNITDATA_REQ", "T_OPTMGMT_REQ", "T_ORDREL_REQ",
		"T_CONN_IND", "T_CONN_CON", "T_DISCON_IND", "T_DATA_IND",
		"T_EXDATA_IND", "T_INFO_ACK", "T_BIND_ACK", "T_ERROR_ACK",
		"T_OK_ACK", "T_UNITDATA_IND", "T_UDERROR_IND",
		"T_OPTMGMT_ACK", "T_ORDREL_IND"
	};

	if (n < 0 || n >= sizeof(tab)/sizeof(tab[0]))
		return "<unknown>";
	return tab[n];
}
#endif


#define timod_mkctl(len) kmalloc(sizeof(struct T_primsg)-sizeof(long)+len, \
					GFP_KERNEL)


static void
timod_socket_wakeup(struct file *filep)
{
	struct socket *sock;

	sock = &filep->f_dentry->d_inode->u.socket_i;

	wake_up_interruptible(&sock->wait);
	if (sock->fasync_list && !(sock->flags & SO_WAITDATA))
		kill_fasync(sock->fasync_list, SIGIO, 0);
}


static void
timod_ok(int fd, int prim)
{
	struct file *filep;
	struct T_primsg *it;

#ifdef CONFIG_ABI_TRACE
	if (ibcs_trace & TRACE_STREAMS)
		printk(KERN_DEBUG "%d iBCS: %u ok ack prim=%d\n",
			current->pid, fd, prim);
#endif
	/* It may not be obvious but we are always holding an fget(fd)
	 * at this point so we can use fcheck(fd) rather than fget...fput.
	 */
	filep = fcheck(fd);

	it = timod_mkctl(sizeof(struct T_ok_ack));
	if (it) {
		struct T_ok_ack *ok = (struct T_ok_ack *)&it->type;
		ok->PRIM_type = T_OK_ACK;
		ok->CORRECT_prim = prim;
		it->pri = MSG_HIPRI;
		it->length = sizeof(struct T_ok_ack);
		it->next = Priv(filep)->pfirst;
		Priv(filep)->pfirst = it;
		if (!Priv(filep)->plast)
			Priv(filep)->plast = it;
		timod_socket_wakeup(filep);
	}
}


static void
timod_error(int fd, int prim, int terr, int uerr)
{
	struct file *filep;
	struct T_primsg *it;

#ifdef CONFIG_ABI_TRACE
	if (ibcs_trace & TRACE_STREAMS)
		printk(KERN_DEBUG "%d iBCS: %u error prim=%d, TLI=%d, UNIX=%d\n",
			current->pid, fd, prim, terr, uerr);
#endif
	/* It may not be obvious but we are always holding an fget(fd)
	 * at this point so we can use fcheck(fd) rather than fget...fput.
	 */
	filep = fcheck(fd);

	it = timod_mkctl(sizeof(struct T_error_ack));
	if (it) {
		struct T_error_ack *err = (struct T_error_ack *)&it->type;
		err->PRIM_type = T_ERROR_ACK;
		err->ERROR_prim = prim;
		err->TLI_error = terr;
		err->UNIX_error = iABI_errors(uerr);
		it->pri = MSG_HIPRI;
		it->length = sizeof(struct T_error_ack);
		it->next = Priv(filep)->pfirst;
		Priv(filep)->pfirst = it;
		if (!Priv(filep)->plast)
			Priv(filep)->plast = it;
		timod_socket_wakeup(filep);
	}
}


#if defined(CONFIG_ABI_XTI_OPTMGMT) || defined(CONFIG_ABI_TLI_OPTMGMT)

#  if defined(CONFIG_ABI_XTI_OPTMGMT) && defined(CONFIG_ABI_TLI_OPTMGMT)
#    error unable to support _both_ TLI and XTI option management
	/* This is because TLI and XTI options buffers are
	 * incompatible and there is no clear way to detect
	 * which format we are dealing with here. Existing
	 * systems appear to have TLI options management
	 * implemented but return TNOTSUPPORT for XTI requests.
	 */
#  endif

static int
timod_optmgmt(int fd, struct pt_regs *regs,
	int flag, char *opt_buf, int opt_len, int do_ret)
{
	struct file *filep;
	int is_tli, error, failed;
	unsigned long old_esp, *tsp;
	char *ret_buf, *ret_base;
	int ret_len, ret_space;

	if (opt_buf && opt_len > 0) {
		error = verify_area(VERIFY_READ, opt_buf, opt_len);
		if (error)
			return error;
	}

	/* It may not be obvious but we are always holding an fget(fd)
	 * at this point so we can use fcheck(fd) rather than fget...fput.
	 */
	filep = fcheck(fd);

	/* FIXME: We should be able to detect the difference between
	 * TLI and XTI requests at run time?
	 */
#ifdef CONFIG_ABI_TLI_OPTMGMT
	is_tli = 1;
#else
	is_tli = 0;
#endif

	if (!do_ret && (!opt_buf || opt_len <= 0))
		return 0;

	/* Grab some space on the user stack to work with. We need 6 longs
	 * to build an argument frame for [gs]etsockopt calls. We also
	 * need space to build the return buffer. This will be at least
	 * as big as the given options buffer but the given options
	 * buffer may not include space for option values so we allow two
	 * longs for each option multiple of the option header size
	 * and hope that big options will not exhaust our space and
	 * trash the stack.
	 */
	ret_space = 1024 + opt_len
		+ 2*sizeof(long)*(opt_len / (is_tli ? sizeof(struct opthdr) : sizeof(struct t_opthdr)));
	ret_buf = ret_base = (char *)(regs->esp - ret_space);
	ret_len = 0;

	old_esp = regs->esp;
	regs->esp -= ret_space + 6*sizeof(long);
	tsp = (unsigned long *)(regs->esp);
	error = verify_area(VERIFY_WRITE, tsp, 6*sizeof(long));
	if (error) {
		regs->esp = old_esp;
		return error;
	}

	failed = 0;

#ifndef CONFIG_ABI_TLI_OPTMGMT
	if (is_tli) {
		printk(KERN_WARNING
			"%d iBCS: TLI optmgmt requested but not supported\n",
			current->pid);
	}
#else
	if (is_tli) while (opt_len >= sizeof(struct opthdr)) {
		struct opthdr opt;

#ifdef CONFIG_ABI_TRACE
		if ((ibcs_trace & TRACE_STREAMS)) {
			printk(KERN_DEBUG "%d iBCS: TLI optmgmt opt_len=%d, "
				"ret_buf=0x%08lx, ret_len=%d, ret_space=%d\n",
				current->pid,
				opt_len, (unsigned long)ret_buf,
				ret_len, ret_space);
		}
#endif
		copy_from_user(&opt, opt_buf, sizeof(struct opthdr));

		/* Idiot check... */
		if (opt.len > opt_len) {
			failed = TBADOPT;
			break;
		}

#ifdef CONFIG_ABI_TRACE
		if ((ibcs_trace & TRACE_STREAMS)) {
			unsigned long v;
			get_user(v, (unsigned long *)(opt_buf+sizeof(struct opthdr)));
			printk(KERN_DEBUG "%d iBCS: TLI optmgmt fd=%d,"
				" level=%ld, name=%ld, value=%ld\n",
				current->pid, fd, opt.level, opt.name, v);
		}
#endif
		/* Check writable space in the return buffer. */
		error = verify_area(VERIFY_WRITE, ret_buf, sizeof(struct opthdr));
		if (error) {
			failed = TSYSERR;
			break;
		}

		/* Flag values:
		 * T_NEGOTIATE means try and set it.
		 * T_DEFAULT means get the default value.
		 *           (return the current for now)
		 * T_CHECK means get the current value.
		 */
		error = 0;
		if (flag == T_NEGOTIATE) {
			put_user(fd, tsp);
			put_user(opt.level, tsp+1);
			put_user(opt.name, tsp+2);
			put_user((int)(opt_buf+sizeof(struct opthdr)), tsp+3);
			put_user(opt.len, tsp+4);
			error = abi_do_setsockopt(tsp);
#ifdef CONFIG_ABI_TRACE
			if (error && (ibcs_trace & TRACE_STREAMS)) {
				printk(KERN_DEBUG
					"%d iBCS: setsockopt failed: %d\n",
					current->pid,
					error);
			}
#endif
			if (error) {
				failed = TBADOPT;
				break;
			}
		}
		if (!error) {
			int len;

			put_user(fd, tsp);
			put_user(opt.level, tsp+1);
			put_user(opt.name, tsp+2);
			put_user((int)(ret_buf+sizeof(struct opthdr)), tsp+3);
			put_user((int)(tsp+5), tsp+4);
			put_user(ret_space, tsp+5);
			error = abi_do_getsockopt(tsp);
#ifdef CONFIG_ABI_TRACE
			if (error && (ibcs_trace & TRACE_STREAMS)) {
				printk(KERN_DEBUG
					"%d iBCS: getsockopt failed: %d\n",
					current->pid,
					error);
			}
#endif
			if (error) {
				failed = TBADOPT;
				break;
			}

			get_user(len, tsp+5);
			copy_to_user(ret_buf, &opt, sizeof(opt));
			put_user(len,
				&((struct opthdr *)opt_buf)->len);
			ret_space -= sizeof(struct opthdr) + len;
			ret_len += sizeof(struct opthdr) + len;
			ret_buf += sizeof(struct opthdr) + len;
		}

		opt_len -= sizeof(struct opthdr) + opt.len;
		opt_buf += sizeof(struct opthdr) + opt.len;
	}
#endif /* CONFIG_ABI_TLI_OPTMGMT */
#ifndef CONFIG_ABI_XTI_OPTMGMT
	else {
		printk(KERN_WARNING
			"%d iBCS: XTI optmgmt requested but not supported\n",
			current->pid);
	}
#else
	else while (opt_len >= sizeof(struct t_opthdr)) {
		struct t_opthdr opt;

		copy_from_user(&opt, opt_buf, sizeof(struct t_opthdr));
		if (opt.len > opt_len) {
			failed = 1;
			break;
		}

#ifdef CONFIG_ABI_TRACE
		if ((ibcs_trace & TRACE_STREAMS)) {
			unsigned long v;
			get_user(v, (unsigned long *)(opt_buf+sizeof(struct t_opthdr)));
			printk(KERN_DEBUG "%d iBCS: XTI optmgmt fd=%d,"
				" level=%ld, name=%ld, value=%ld\n",
				current->pid, fd, opt.level, opt.name, v);
		}
#endif
		/* Check writable space in the return buffer. */
		if (verify_area(VERIFY_WRITE, ret_buf, sizeof(struct t_opthdr))) {
			failed = 1;
			break;
		}

		/* Flag values:
		 * T_NEGOTIATE means try and set it.
		 * T_CHECK means see if we could set it.
		 *         (so we just set it for now)
		 * T_DEFAULT means get the default value.
		 *           (return the current for now)
		 * T_CURRENT means get the current value (SCO xti.h has
		 * no T_CURRENT???).
		 */
		error = 0;
		if (flag == T_NEGOTIATE || flag == T_CHECK) {
			put_user(fd, tsp);
			put_user(opt.level, tsp+1);
			put_user(opt.name, tsp+2);
			put_user((int)(opt_buf+sizeof(struct t_opthdr)), tsp+3);
			put_user(opt.len-sizeof(struct t_opthdr), tsp+4);
			error = abi_do_setsockopt(tsp);
		}
		if (!error) {
			put_user(fd, tsp);
			put_user(opt.level, tsp+1);
			put_user(opt.name, tsp+2);
			put_user((int)(ret_buf+sizeof(struct t_opthdr)), tsp+3);
			put_user((int)(tsp+5), tsp+4);
			put_user(ret_space, tsp+5);
			error = abi_do_getsockopt(tsp);
			if (!error) {
				int len;
				get_user(len, tsp+5);
				/* FIXME: opt.status should be set... */
				copy_to_user(ret_buf, &opt, sizeof(opt));
				put_user(len+sizeof(struct t_opthdr),
					&((struct t_opthdr *)opt_buf)->len);
				ret_space -= sizeof(struct t_opthdr) + len;
				ret_len += sizeof(struct t_opthdr) + len;
				ret_buf += sizeof(struct t_opthdr) + len;
			}
		}

		failed |= error;
		opt_len -= opt.len;
		opt_buf += opt.len;
	}
#endif /* CONFIG_ABI_XTI_OPTMGMT */

#if 0
	/* If there is left over data the supplied options buffer was
	 * formatted incorrectly. But we might have done some work so
	 * we must fall through and return an acknowledgement I think.
	 */
	if (opt_len) {
		regs->esp = old_esp;
		return -EINVAL;
	}
#endif

	if (do_ret) {
		struct T_primsg *it;

		if (failed) {
			timod_error(fd, T_OPTMGMT_REQ, failed, -error);
			regs->esp = old_esp;
			return 0;
		}

#ifdef CONFIG_ABI_TRACE
		if ((ibcs_trace & TRACE_STREAMS)) {
			printk(KERN_DEBUG "%d iBCS: optmgmt returns %d bytes,"
				" failed=%d\n",
				current->pid, ret_len, failed);
		}
#endif
		/* Convert the return buffer in the user stack to a
		 * T_OPTMGMT_ACK
		 * message and queue it.
		 */
		it = timod_mkctl(sizeof(struct T_optmgmt_ack) + ret_len);
		if (it) {
			struct T_optmgmt_ack *ack
				= (struct T_optmgmt_ack *)&it->type;
			ack->PRIM_type = T_OPTMGMT_ACK;
			ack->OPT_length = ret_len;
			ack->OPT_offset = sizeof(struct T_optmgmt_ack);
			ack->MGMT_flags = (failed ? T_FAILURE : flag);
			copy_from_user(((char *)ack)+sizeof(struct T_optmgmt_ack),
				ret_base, ret_len);
			it->pri = MSG_HIPRI;
			it->length = sizeof(struct T_optmgmt_ack) + ret_len;
			it->next = Priv(filep)->pfirst;
			Priv(filep)->pfirst = it;
			if (!Priv(filep)->plast)
				Priv(filep)->plast = it;
			timod_socket_wakeup(filep);
		}
	}

	regs->esp = old_esp;
	return 0;
}
#else /* no CONFIG_ABI_XTI_OPTMGMT or CONFIG_ABI_TLI_OPTMGMT */
static int
timod_optmgmt(int fd, struct pt_regs *regs,
	int flag, char *opt_buf, int opt_len, int do_ret)
{
	return -EINVAL;
}
#endif /* CONFIG_ABI_XTI_OPTMGMT or CONFIG_ABI_TLI_OPTMGMT */


static inline void
free_wait(poll_table *p)
{
	struct poll_table_entry *entry = p->entry + p->nr;

	while (p->nr > 0) {
		p->nr--;
		entry--;
		remove_wait_queue(entry->wait_address,&entry->wait);
		fput(entry->filp);
	}
}


int
timod_update_socket(int fd, struct file *filep, struct pt_regs *regs)
{
	int error;

	/* If this a SOCK_STREAM and is in the TS_WRES_CIND state
	 * we are supposed to be looking for an incoming connection.
	 */
	if (filep->f_dentry->d_inode->u.socket_i.type == SOCK_STREAM
	&& Priv(filep)->state == TS_WRES_CIND) {
		unsigned long old_esp, *tsp;
		unsigned short oldflags;

		old_esp = regs->esp;
		regs->esp -= 1024;
		tsp = (unsigned long *)regs->esp;
		error = verify_area(VERIFY_WRITE, tsp,
				3*sizeof(long)+sizeof(struct sockaddr));
		if (error) {
			regs->esp = old_esp;
			return error;
		}

		put_user(fd, tsp);
		put_user((unsigned long)(tsp+4), tsp+1);
		put_user((unsigned long)(tsp+3), tsp+2);
		put_user(sizeof(struct sockaddr), tsp+3);

		/* We don't want to block in the accept(). Any
		 * blocking necessary must be handled earlier.
		 */
		oldflags = filep->f_flags;
		filep->f_flags |= O_NONBLOCK;
		error = SYS(socketcall)(SYS_ACCEPT, tsp);
		filep->f_flags = oldflags;

		if (error >= 0) {
			unsigned long alen;
			struct T_primsg *it;

			/* The new fd needs to be fixed up
			 * with the iBCS file functions and a
			 * timod state block.
			 */
			inherit_socksys_funcs(error, TS_DATA_XFER);

			/* Generate a T_CONN_IND and queue it. */
			get_user(alen, tsp+3);
			it = timod_mkctl(sizeof(struct T_conn_ind) + alen);
			if (!it) {
				/* Oops, just drop the connection I guess. */
				SYS(close)(error);
			} else {
				struct T_conn_ind *ind
					= (struct T_conn_ind *)&it->type;
				ind->PRIM_type = T_CONN_IND;
				ind->SRC_length = alen;
				ind->SRC_offset = sizeof(struct T_conn_ind);
				ind->OPT_length = ind->OPT_offset = 0;
				ind->SEQ_number = error;
				copy_from_user(((char *)ind)+sizeof(struct T_conn_ind),
					tsp+4, alen);
#if 0
				it->pri = MSG_HIPRI;
#endif
				it->length = sizeof(struct T_conn_ind) + alen;
				it->next = Priv(filep)->pfirst;
				Priv(filep)->pfirst = it;
				if (!Priv(filep)->plast)
					Priv(filep)->plast = it;
				timod_socket_wakeup(filep);
			}
		}
		regs->esp = old_esp;
	}
	return 0;
}


static int
do_getmsg(int fd, struct pt_regs *regs,
	char *ctl_buf, int ctl_maxlen, int *ctl_len,
	char *dat_buf, int dat_maxlen, int *dat_len,
	int *flags_p)
{
	int error;
	long old_esp;
	unsigned long *tsp;
	unsigned short oldflags;
	struct T_unitdata_ind udi;
	struct file *filep;

	/* It may not be obvious but we are always holding an fget(fd)
	 * at this point so we can use fcheck(fd) rather than fget...fput.
	 */
	filep = fcheck(fd);

	if (!Priv(filep) && Priv(filep)->magic != XTI_MAGIC) {
		printk("putmsg on non-STREAMS fd %d by %s\n",fd, current->comm);
		return -EINVAL;
	}

#ifdef CONFIG_ABI_TRACE
	if ((ibcs_trace & TRACE_STREAMS) || ibcs_func_p->trace) {
		printk(KERN_DEBUG "%d iBCS: getmsg %d, 0x%lx[%d], 0x%lx[%d], %x\n",
			current->pid,
			fd,
			(unsigned long)ctl_buf, ctl_maxlen,
			(unsigned long)dat_buf, dat_maxlen,
			*flags_p);
	}
#endif

	/* We need some user space to build syscall argument vectors
	 * later. Set it up now and page it in if necessary. This will
	 * avoid (most?) potential blocking after the select().
	 */
	old_esp = regs->esp;
	regs->esp -= 1024;
	tsp = (unsigned long *)regs->esp;
	error = verify_area(VERIFY_WRITE, tsp, 6*sizeof(long));
	regs->esp = old_esp;
	if (error)
		return error;

	/* If the TEP is not non-blocking we must test for
	 * something to do. We don't necessarily know what order
	 * events will be happening on the socket so we have to
	 * watch for evrything at once.
	 * N.B. If we weren't asked for data we should only be looking
	 * for connection requests. There are socket type issues to
	 * consider here.
	 */
	if (!(filep->f_flags & O_NONBLOCK)) {
		poll_table wait_table, *wait;
		struct poll_table_entry *entry;
		unsigned long mask = (POLLIN | POLLRDNORM | POLLHUP | POLLERR);

		if (*flags_p == MSG_HIPRI)
			mask |= POLLPRI;

		entry = (struct poll_table_entry *)__get_free_page(GFP_KERNEL);
		if (!entry)
			return -ENOMEM;
		wait_table.nr = 0;
		wait_table.entry = entry;
		wait = &wait_table;

		/* N.B. We need to be sure to recheck after a schedule()
		 * so that when we proceed it is because there is
		 * something to do and nothing else can get there
		 * before us.
		 */
		while (!(filep->f_op->poll(filep, wait) & mask)
		&& !signal_pending(current)) {
			current->state = TASK_INTERRUPTIBLE;
			wait = NULL;
			schedule();
		}
		current->state = TASK_RUNNING;
		free_wait(&wait_table);
		free_page((unsigned long)wait_table.entry);

		if (signal_pending(current))
			return -EINTR;
	}

	if (ctl_maxlen >= 0 && !Priv(filep)->pfirst)
		timod_update_socket(fd, filep, regs);

	/* If we were asked for a control part and there is an outstanding
	 * message queued as a result of some other operation we'll
	 * return that.
	 */
	if (ctl_maxlen >= 0 && Priv(filep)->pfirst) {
		int l = ctl_maxlen <= Priv(filep)->pfirst->length
				? ctl_maxlen : Priv(filep)->pfirst->length;
		error = verify_area(VERIFY_WRITE, ctl_buf, l);
		if (error)
			return error;
#ifdef CONFIG_ABI_TRACE
		if ((ibcs_trace & TRACE_STREAMS) || ibcs_func_p->trace) {
			printk(KERN_DEBUG "%d iBCS: priority message %ld %s\n",
				current->pid,
				Priv(filep)->pfirst->type,
				xti_prim(Priv(filep)->pfirst->type));
		}
#endif
		copy_to_user(ctl_buf, ((char *)&Priv(filep)->pfirst->type)
					+ Priv(filep)->offset, l);
		put_user(l, ctl_len);
		if (dat_maxlen >= 0)
			put_user(0, dat_len);
		*flags_p = Priv(filep)->pfirst->pri;
		Priv(filep)->pfirst->length -= l;
#if 1
#ifdef CONFIG_ABI_TRACE
if ((ibcs_trace & TRACE_STREAMS) || ibcs_func_p->trace) {
if (ctl_buf && l > 0) { int i = -1;
for (i=0; i<l && i<64; i+=4) {
unsigned long v;
get_user(v, (unsigned long *)(ctl_buf+i));
printk(KERN_DEBUG "%d iBCS: ctl: 0x%08lx\n", current->pid, v);
}
if (i != l) printk(KERN_DEBUG "%d iBCS: ctl: ...\n", current->pid);
}
}
#endif
#endif
		if (Priv(filep)->pfirst->length) {
			Priv(filep)->offset += l;
#ifdef CONFIG_ABI_TRACE
			if ((ibcs_trace & TRACE_STREAMS)
			|| ibcs_func_p->trace) {
				printk(KERN_DEBUG
					"%d iBCS: MORECTL %d bytes\n",
					current->pid,
					Priv(filep)->pfirst->length);
			}
#endif
			return MORECTL;
		} else {
			struct T_primsg *it = Priv(filep)->pfirst;
			Priv(filep)->pfirst = it->next;
			if (!Priv(filep)->pfirst)
				Priv(filep)->plast = NULL;
			kfree(it);
			Priv(filep)->offset = 0;
			return 0;
		}
	}

	*flags_p = 0;

	/* If we weren't asked for data there is nothing more to do. */
	if (dat_maxlen <= 0) {
		if (dat_maxlen == 0)
			put_user(0, dat_len);
		if (ctl_maxlen >= 0)
			put_user(0, ctl_len);
		return -EAGAIN;
	}

	/* If the select() slept we may have had our temp space paged
	 * out. The re-verify_area is only really needed for pre-486
	 * chips which don't handle write faults from kernel mode.
	 */
	regs->esp = (unsigned long)tsp;
	error = verify_area(VERIFY_WRITE, tsp, 6*sizeof(long));
	if (error) {
		regs->esp = old_esp;
		return error;
	}
	put_user(fd, tsp);
	put_user((unsigned long)dat_buf, tsp+1);
	put_user((dat_maxlen < 0 ? 0 : dat_maxlen), tsp+2);
	put_user(0, tsp+3);
	if (ctl_maxlen > (int)sizeof(udi) && Priv(filep)->state == TS_IDLE) {
		put_user((unsigned long)ctl_buf+sizeof(udi), tsp+4);
		put_user(ctl_maxlen-sizeof(udi), ctl_len);
		put_user((int)ctl_len, tsp+5);
	} else {
		put_user(0, tsp+4);
		put_user(0, ctl_len);
		put_user((int)ctl_len, tsp+5);
	}

	/* We don't want to block in the recvfrom(). Any blocking is
	 * handled by the select stuff above.
	 */
	oldflags = filep->f_flags;
	filep->f_flags |= O_NONBLOCK;
	error = SYS(socketcall)(SYS_RECVFROM, tsp);
	filep->f_flags = oldflags;

	regs->esp = old_esp;
	if (error < 0)
		return error;
	if (error
	&& ctl_maxlen > (int)sizeof(udi)
	&& Priv(filep)->state == TS_IDLE) {
		udi.PRIM_type = T_UNITDATA_IND;
		get_user(udi.SRC_length, ctl_len);
		udi.SRC_offset = sizeof(udi);
		udi.OPT_length = udi.OPT_offset = 0;
		copy_to_user(ctl_buf, &udi, (int)sizeof(udi));
		put_user(sizeof(udi)+udi.SRC_length, ctl_len);
#if 0
#ifdef CONFIG_ABI_TRACE
if ((ibcs_trace & TRACE_STREAMS) || ibcs_func_p->trace) {
if (ctl_buf && udi.SRC_length > 0) { int i = -1;
char *buf = ctl_buf + sizeof(udi);
for (i=0; i<udi.SRC_length && i<64; i+=4) {
unsigned long v;
get_user(v, (unsigned long *)(buf+i));
printk(KERN_debug "%d iBCS: dat: 0x%08lx\n", current->pid, v);
}
if (i != udi.SRC_length) printk(KERN_DEBUG "%d iBCS: dat: ...\n", current->pid);
}
}
#endif
#endif
	} else {
		put_user(0, ctl_len);
	}
	put_user(error, dat_len);

	return 0;
}


static int
do_putmsg(int fd, struct pt_regs *regs, char *ctl_buf, int ctl_len,
	char *dat_buf, int dat_len, int flags)
{
	struct file *filep;
	int error, terror;
	unsigned long cmd;

	/* It may not be obvious but we are always holding an fget(fd)
	 * at this point so we can use fcheck(fd) rather than fget...fput.
	 */
	filep = fcheck(fd);

	if (!Priv(filep) && Priv(filep)->magic != XTI_MAGIC) {
		printk("putmsg on non-STREAMS fd %d by %s\n",fd, current->comm);
		return -EINVAL;
	}
#ifdef CONFIG_ABI_TRACE
	if ((ibcs_trace & TRACE_STREAMS) || ibcs_func_p->trace) {
		unsigned long v;
		printk(KERN_DEBUG "%d iBCS: putmsg %d, 0x%lx[%d], 0x%lx[%d], %x\n",
			current->pid,
			fd,
			(unsigned long)ctl_buf, ctl_len,
			(unsigned long)dat_buf, dat_len,
			flags);
		get_user(v, ctl_buf);
		printk(KERN_DEBUG "%d iBCS: putmsg prim: %ld %s\n",
			current->pid, v, xti_prim(v));
	}
#if 1
if ((ibcs_trace & TRACE_STREAMS) || ibcs_func_p->trace) {
if (ctl_buf && ctl_len > 0) { int i = -1;
for (i=0; i<ctl_len && i<64; i+=4) {
unsigned long v;
get_user(v, (unsigned long *)(ctl_buf+i));
printk(KERN_DEBUG "%d iBCS: ctl: 0x%08lx\n", current->pid, v);
}
if (i != ctl_len) printk(KERN_DEBUG "%d iBCS: ctl: ...\n", current->pid);
}
if (dat_buf && dat_len > 0) { int i = -1;
for (i=0; i<dat_len && i<64; i+=4) {
unsigned long v;
get_user(v, (unsigned long *)(dat_buf+i));
printk(KERN_DEBUG "%d iBCS: dat: 0x%08lx\n", current->pid, v);
}
if (i != dat_len) printk(KERN_DEBUG "%d iBCS: dat: ...\n", current->pid);
}
}
#endif
#endif

	error = get_user(cmd, (unsigned long *)ctl_buf);
	if (error)
		return error;

	switch (cmd) {
		case T_BIND_REQ: {
			struct T_bind_req req;
			long old_esp;
			unsigned long *tsp;

#ifdef CONFIG_ABI_TRACE
			if (ibcs_trace & TRACE_STREAMS)
				printk(KERN_DEBUG "%d iBCS: %u bind req\n",
					current->pid, fd);
#endif
			error = verify_area(VERIFY_READ, ctl_buf, sizeof(req));
			if (error)
				return error;

			if (Priv(filep)->state != TS_UNBND) {
				timod_error(fd, T_BIND_REQ, TOUTSTATE, 0);
				return 0;
			}

			old_esp = regs->esp;
			regs->esp -= 1024;
			tsp = (unsigned long *)(regs->esp);
			error = verify_area(VERIFY_WRITE, tsp, 3*sizeof(long));
			if (error) {
				timod_error(fd, T_BIND_REQ, TSYSERR, -error);
				regs->esp = old_esp;
				return 0;
			}

			copy_from_user(&req, ctl_buf, sizeof(req));
			if (req.ADDR_offset && req.ADDR_length) {
				struct sockaddr_in *sin;
				unsigned short family;

#if 1				/* Wheee... Kludge time... */
				sin = (struct sockaddr_in *)(ctl_buf
					+ req.ADDR_offset);
				get_user(family, &sin->sin_family);

				/* Sybase seems to have set up the address
				 * struct with sa->sa_family = htons(AF_?)
				 * which is bollocks. I have no idea why it
				 * apparently works on SCO?!?
				 */
				if (family && !(family & 0x00ff))
					put_user(ntohs(family), &sin->sin_family);
#endif

				put_user(fd, tsp);
				put_user((unsigned long)ctl_buf
						+ req.ADDR_offset, tsp+1);
				/* For TLI/XTI the length may be the 8 *used*
				 * bytes, for (IP?) sockets it must be the 16
				 * *total* bytes in a sockaddr_in.
				 */
				put_user(req.ADDR_length == 8
					? 16 : req.ADDR_length,
					tsp+2);
				error = SYS(socketcall)(SYS_BIND, tsp);

				if (!error) {
					if (req.CONIND_number) {
#ifdef CONFIG_ABI_TRACE
						if (ibcs_trace & TRACE_STREAMS) {
							printk(KERN_DEBUG
								"%d iBCS: %u listen backlog=%lu\n",
								current->pid,
								fd, req.CONIND_number);
						}
#endif
						put_user(fd, tsp);
						put_user(req.CONIND_number, tsp+1);
						SYS(socketcall)(SYS_LISTEN, tsp);
						Priv(filep)->state = TS_WRES_CIND;
					} else {
						Priv(filep)->state = TS_IDLE;
					}
				}
			} else {
				error = 0;
			}

			regs->esp = old_esp;

			if (!error) {
				struct T_primsg *it;
				it = timod_mkctl(ctl_len);
				if (it) {
					struct T_bind_ack *ack = (struct T_bind_ack *)&it->type;
					copy_from_user(ack, ctl_buf, ctl_len);
					ack->PRIM_type = T_BIND_ACK;
					it->pri = MSG_HIPRI;
					it->length = ctl_len;
					it->next = NULL;
					timod_ok(fd, T_BIND_REQ);
					Priv(filep)->plast->next = it;
					Priv(filep)->plast = it;
					return 0;
				}
			}
			switch (error) {
				case -EINVAL:
					terror = TOUTSTATE;
					error = 0;
					break;
				case -EACCES:
					terror = TACCES;
					error = 0;
					break;
				case -EADDRNOTAVAIL:
				case -EADDRINUSE:
					terror = TNOADDR;
					error = 0;
					break;
				default:
					terror = TSYSERR;
					break;
			}
			timod_error(fd, T_BIND_REQ, terror, -error);
			return 0;
		}
		case T_CONN_RES: {
			struct T_conn_res *res = (struct T_conn_res *)ctl_buf;
			unsigned int conn_fd;

			error = get_user(conn_fd, &res->SEQ_number);
			if (error)
				return error;
#ifdef CONFIG_ABI_TRACE
			if (ibcs_trace & TRACE_STREAMS)
				printk(KERN_DEBUG "%d iBCS: %u accept:"
					"conn fd=%u, use fd=%u\n",
					current->pid, fd, conn_fd, flags);
#endif
			if (conn_fd != flags) {
				error = SYS(dup2)(conn_fd, flags);
				SYS(close)(conn_fd);
				if (error < 0)
					return error;
			}
			timod_ok(fd, T_CONN_RES);
			return 0;
		}
		case T_CONN_REQ: {
			struct T_conn_req req;
			long old_esp;
			unsigned short oldflags;
			unsigned long *tsp;
			struct T_primsg *it;
			struct sockaddr_in *sin;
			unsigned short family;

#ifdef CONFIG_ABI_TRACE
			if (ibcs_trace & TRACE_STREAMS)
				printk(KERN_DEBUG "%d iBCS: %u connect req\n",
					current->pid, fd);
#endif
			error = verify_area(VERIFY_READ, ctl_buf, sizeof(req));
			if (error)
				return error;

			if (Priv(filep)->state != TS_UNBND
			&& Priv(filep)->state != TS_IDLE) {
				timod_error(fd, T_CONN_REQ, TOUTSTATE, 0);
				return 0;
			}

			old_esp = regs->esp;
			regs->esp -= 1024;
			tsp = (unsigned long *)(regs->esp);
			error = verify_area(VERIFY_WRITE, tsp, 3*sizeof(long));
			if (error) {
				timod_error(fd, T_CONN_REQ, TSYSERR, -error);
				regs->esp = old_esp;
				return 0;
			}
			copy_from_user(&req, ctl_buf, sizeof(req));
			put_user(fd, tsp);
			put_user((unsigned long)ctl_buf + req.DEST_offset, tsp+1);
			/* For TLI/XTI the length may be the 8 *used*
			 * bytes, for (IP?) sockets it must be the 16
			 * *total* bytes in a sockaddr_in.
			 */
			put_user(req.DEST_length == 8
				? 16 : req.DEST_length,
				tsp+2);

#if 1			/* Wheee... Kludge time... */
			sin = (struct sockaddr_in *)(ctl_buf
				+ req.DEST_offset);
			get_user(family, &sin->sin_family);

			/* Sybase seems to have set up the address
			 * struct with sa->sa_family = htons(AF_?)
			 * which is bollocks. I have no idea why it
			 * apparently works on SCO?!?
			 */
			if (family && !(family & 0x00ff)) {
				family = ntohs(family);
				put_user(family, &sin->sin_family);
			}

			/* Sheesh... ISC telnet seems to give the port
			 * number low byte first as I expected but the
			 * X programs seem to be giving high byte first.
			 * One is broken of course but clearly both
			 * should work. No, I don't understand this
			 * either but I can at least try...
			 * A better solution would be for you to change
			 * the definition of xserver0 in ISC's /etc/services
			 * but then it wouldn't work out of the box...
			 */
			if (personality(PER_SVR4) && family == AF_INET) {
				get_user(family, &sin->sin_port);
				if (family == 0x1770)
					put_user(htons(family),
						&sin->sin_port);
			}
#endif
			/* FIXME: We should honour non-blocking mode
			 * here but that means that the select probe
			 * needs to know that if select returns ok and
			 * we are in T_OUTCON we have a connection
			 * completion. This isn't so bad but the real
			 * problem is that the connection acknowledgement
			 * is supposed to contain the destination
			 * address.
			 */
			oldflags = filep->f_flags;
			filep->f_flags &= ~O_NONBLOCK;
			error = SYS(socketcall)(SYS_CONNECT, tsp);
			filep->f_flags = oldflags;
			regs->esp = old_esp;

			if (!error) {
				struct T_conn_con *con;

				it = timod_mkctl(ctl_len);
				if (!it)
					return -ENOMEM;
				it->length = ctl_len;
				con = (struct T_conn_con *)&it->type;
				copy_from_user(con, ctl_buf, ctl_len);
				con->PRIM_type = T_CONN_CON;
				Priv(filep)->state = TS_DATA_XFER;
			} else {
				struct T_discon_ind *dis;

#ifdef CONFIG_ABI_TRACE
				if (ibcs_trace & TRACE_STREAMS)
					printk(KERN_DEBUG "%d iBCS: %u "
						"connect failed (errno=%d)\n",
						current->pid, fd,
						error);
#endif
				it = timod_mkctl(sizeof(struct T_discon_ind));
				if (!it)
					return -ENOMEM;
				it->length = sizeof(struct T_discon_ind);
				dis = (struct T_discon_ind *)&it->type;
				dis->PRIM_type = T_DISCON_IND;
				dis->DISCON_reason = iABI_errors(-error);
				dis->SEQ_number = 0;
			}
			timod_ok(fd, T_CONN_REQ);
			it->pri = 0;
			it->next = NULL;
			Priv(filep)->plast->next = it;
			Priv(filep)->plast = it;
			return 0;
		}

		case T_DISCON_REQ: {
			struct T_discon_req *req;

			req = (struct T_discon_req *)ctl_buf;
			error = get_user(fd, &req->SEQ_number);
			if (error)
				return error;
#ifdef CONFIG_ABI_TRACE
			if (ibcs_trace & TRACE_STREAMS)
				printk(KERN_DEBUG "%d iBCS: disconnect %u\n",
					current->pid, fd);
#endif
			/* Fall through... */
		}
		case T_ORDREL_REQ: {
			SYS(close)(fd);
			return 0;
		}

		case T_DATA_REQ: {
			long old_esp;
			unsigned long *tsp;

#ifdef CONFIG_ABI_TRACE
			if (ibcs_trace & TRACE_STREAMS)
				printk(KERN_DEBUG "%d iBCS: %u data req\n",
					current->pid, fd);
#endif
			if (Priv(filep)->state != TS_DATA_XFER) {
				return 0;
			}

			old_esp = regs->esp;
			regs->esp -= 1024;
			tsp = (unsigned long *)(regs->esp);
			error = verify_area(VERIFY_WRITE, tsp, 6*sizeof(long));
			if (error) {
				regs->esp = old_esp;
				return 0;
			}
			put_user(fd, tsp);
			put_user((unsigned long)dat_buf, tsp+1);
			put_user(dat_len, tsp+2);
			put_user(0, tsp+3);
			error = SYS(socketcall)(SYS_SEND, tsp);
			regs->esp = old_esp;
			return error;
		}

		case T_UNITDATA_REQ: {
			struct T_unitdata_req req;
			long old_esp;
			unsigned long *tsp;

#ifdef CONFIG_ABI_TRACE
			if (ibcs_trace & TRACE_STREAMS)
				printk(KERN_DEBUG "%d iBCS: %u unitdata req\n",
					current->pid, fd);
#endif
			error = verify_area(VERIFY_READ, ctl_buf, sizeof(req));
			if (error)
				return error;

			if (Priv(filep)->state != TS_IDLE
			&& Priv(filep)->state != TS_DATA_XFER) {
				timod_error(fd, T_UNITDATA_REQ, TOUTSTATE, 0);
				return 0;
			}

			old_esp = regs->esp;
			regs->esp -= 1024;
			tsp = (unsigned long *)(regs->esp);
			error = verify_area(VERIFY_WRITE, tsp, 6*sizeof(long));
			if (error) {
				timod_error(fd, T_UNITDATA_REQ, TSYSERR, -error);
				regs->esp = old_esp;
				return 0;
			}
			put_user(fd, tsp);
			put_user((unsigned long)dat_buf, tsp+1);
			put_user(dat_len, tsp+2);
			put_user(0, tsp+3);
			copy_from_user(&req, ctl_buf, sizeof(req));
			if (req.DEST_length > 0) {
				put_user((unsigned long)(ctl_buf+req.DEST_offset), tsp+4);
				put_user(req.DEST_length, tsp+5);
				error = SYS(socketcall)(SYS_SENDTO, tsp);
				regs->esp = old_esp;
				return error;
			}
			error = SYS(socketcall)(SYS_SEND, tsp);
			regs->esp = old_esp;
			return error;
		}

		case T_UNBIND_REQ:
			Priv(filep)->state = TS_UNBND;
			timod_ok(fd, T_UNBIND_REQ);
			return 0;

		case T_OPTMGMT_REQ: {
			struct T_optmgmt_req req;
#ifdef CONFIG_ABI_TRACE
			if (ibcs_trace & TRACE_STREAMS)
				printk(KERN_DEBUG "%d iBCS: %u optmgmt req\n",
					current->pid, fd);
#endif
			error = verify_area(VERIFY_READ, ctl_buf, sizeof(req));
			if (error)
				return error;
			copy_from_user(&req, ctl_buf, sizeof(req));

			return timod_optmgmt(fd, regs, req.MGMT_flags,
					req.OPT_offset > 0
						? ctl_buf+req.OPT_offset
						: NULL,
					req.OPT_length,
					1);
		}
	}
#if CONFIG_ABI_TRACE
	if (ctl_buf && ctl_len > 0) {
		int i;
		for (i=0; i<ctl_len && i<32; i+=4) {
			unsigned long v;
			get_user(v, (unsigned long *)(ctl_buf+i));
			printk(KERN_DEBUG "%d iBCS: ctl: 0x%08lx\n",
				current->pid, v);
		}
		if (i != ctl_len)
			printk(KERN_DEBUG "%d iBCS: ctl: ...\n",
				current->pid);
	}
	if (dat_buf && dat_len > 0) {
		int i;
		for (i=0; i<dat_len && i<32; i+=4) {
			unsigned long v;
			get_user(v, (unsigned long *)(dat_buf+i));
			printk(KERN_DEBUG "%d iBCS: dat: 0x%08lx\n",
				current->pid, v);
		}
		if (i != dat_len)
			printk(KERN_DEBUG "%d iBCS: dat: ...\n",
				current->pid);
	}
#endif
	return -EINVAL;
}


int
timod_ioctl(struct pt_regs *regs,
	int fd, unsigned int func, void *arg, int len, int *len_p)
{
	struct file *filep;
	struct inode *ino;
	int error;

	filep = fget(fd);
	if (!filep)
		return TBADF;

	error = verify_area(VERIFY_WRITE, len_p, sizeof(int));
	if (error) {
		fput(filep);
		return (-error << 8) | TSYSERR;
	}

	ino = filep->f_dentry->d_inode;

	/* SCO/SVR3 starts at 100, ISC/SVR4 starts at 140. */
	switch (func >= 140 ? func-140 : func-100) {
		case 0: /* TI_GETINFO */
		{
			struct T_info_ack it;
			unsigned long v;

#ifdef CONFIG_ABI_TRACE
			if (ibcs_trace & TRACE_STREAMS)
				printk(KERN_DEBUG "%d iBCS: %u getinfo\n",
					current->pid, fd);
#endif
			/* The pre-SVR4 T_info_ack structure didn't have
			 * the PROVIDER_flag on the end.
			 */
			error = verify_area(VERIFY_WRITE, arg,
				func == 140
				? sizeof(struct T_info_ack)
				: sizeof(struct T_info_ack)-sizeof(long));
			if (error) {
				fput(filep);
				return (-error << 8) | TSYSERR;
			}

			__get_user(v, &((struct T_info_req *)arg)->PRIM_type);
			if (v != T_INFO_REQ) {
				fput(filep);
				return (EINVAL << 8) | TSYSERR;
			}

			it.PRIM_type = T_INFO_ACK;
			it.CURRENT_state = Priv(filep)->state;
			it.CDATA_size = -2;
			it.DDATA_size = -2;
			it.OPT_size = -1;
			it.TIDU_size = 16384;
			switch ((MINOR(ino->i_rdev)>>4) & 0x0f) {
				case AF_UNIX:
					it.ADDR_size = sizeof(struct sockaddr_un);
					break;
				case AF_INET:
					it.ADDR_size = sizeof(struct sockaddr_in);
					break;
				default:
					/* Uh... dunno... play safe(?) */
					it.ADDR_size = 1024;
					break;
			}
			switch (ino->u.socket_i.type) {
				case SOCK_STREAM:
					it.ETSDU_size = 1;
					it.TSDU_size = 0;
					it.SERV_type = 2;
					break;
				default:
					it.ETSDU_size = -2;
					it.TSDU_size = 16384;
					it.SERV_type = 3;
					break;
			}

			fput(filep);

			/* The pre-SVR4 T_info_ack structure didn't have
			 * the PROVIDER_flag on the end.
			 */
			if (func == 140) {
				it.PROVIDER_flag = 0;
				copy_to_user(arg, &it, sizeof(it));
				put_user(sizeof(it), len_p);
				return 0;
			}
			copy_to_user(arg, &it, sizeof(it)-sizeof(long));
			put_user(sizeof(it)-sizeof(long), len_p);
			return 0;
		}

		case 2: /* TI_BIND */
		{
			int i;
			long prim;

#ifdef CONFIG_ABI_TRACE
			if (ibcs_trace & TRACE_STREAMS)
				printk(KERN_DEBUG "%d iBCS: %u bind\n",
					current->pid, fd);
#endif
			error = do_putmsg(fd, regs, arg, len,
					NULL, -1, 0);
			if (error) {
				fput(filep);
				return (-error << 8) | TSYSERR;
			}

			/* Get the response. This should be either
			 * T_OK_ACK or T_ERROR_ACK.
			 */
			i = MSG_HIPRI;
			error = do_getmsg(fd, regs,
					arg, len, len_p,
					NULL, -1, NULL,
					&i);
			if (error) {
				fput(filep);
				return (-error << 8) | TSYSERR;
			}

			get_user(prim, (unsigned long *)arg);
			if (prim == T_ERROR_ACK) {
				unsigned long a, b;
				fput(filep);
				get_user(a, ((unsigned long *)arg)+3);
				get_user(b, ((unsigned long *)arg)+2);
				return (a << 8) | b;
			}
			if (prim != T_OK_ACK) {
				fput(filep);
				return TBADSEQ;
			}

			/* Get the response to the bind request. */
			i = MSG_HIPRI;
			error = do_getmsg(fd, regs,
					arg, len, len_p,
					NULL, -1, NULL,
					&i);
			fput(filep);
			if (error)
				return (-error << 8) | TSYSERR;

			return 0;
		}

		case 3: /* TI_UNBIND */
			if (Priv(filep)->state != TS_IDLE) {
				fput(filep);
				return TOUTSTATE;
			}
			Priv(filep)->state = TS_UNBND;
			fput(filep);
			return 0;

		case 1: { /* TI_OPTMGMT */
#if defined(CONFIG_ABI_XTI_OPTMGMT) || defined(CONFIG_ABI_TLI_OPTMGMT)
			int i;
			long prim;

#ifdef CONFIG_ABI_TRACE
			if (ibcs_trace & TRACE_STREAMS)
				printk(KERN_DEBUG "%d iBCS: %u optmgmt\n",
					current->pid, fd);
#endif
			error = do_putmsg(fd, regs, arg, len,
					NULL, -1, 0);
			if (error) {
				fput(filep);
				return (-error << 8) | TSYSERR;
			}

			/* Get the response to the optmgmt request. */
			i = MSG_HIPRI;
			error = do_getmsg(fd, regs,
					arg, len, len_p,
					NULL, -1, NULL,
					&i);
			if (error > 0) {
				/* If there is excess data in the response
				 * our buffer is too small which implies
				 * the application is broken. SO_LINGER
				 * is a common fault. Because it works
				 * on other systems we attempt to recover
				 * by discarding the excess.
				 */
				struct T_primsg *it = Priv(filep)->pfirst;
				Priv(filep)->pfirst = it->next;
				if (!Priv(filep)->pfirst)
					Priv(filep)->plast = NULL;
				kfree(it);
				Priv(filep)->offset = 0;
#ifdef CONFIG_ABI_TRACE
				if ((ibcs_trace & TRACE_STREAMS)
				|| ibcs_func_p->trace) {
					printk(KERN_DEBUG
						"%d iBCS: excess discarded\n",
						current->pid);
				}
#endif
			}

			fput(filep);

			if (error < 0)
				return (-error << 8) | TSYSERR;

			__get_user(prim, (unsigned long *)arg);
			if (prim == T_ERROR_ACK) {
				unsigned long a, b;
				__get_user(a, ((unsigned long *)arg)+3);
				__get_user(b, ((unsigned long *)arg)+2);
				return (a << 8) | b;
			}

			return 0;
#else /* no CONFIG_ABI_XTI_OPTMGMT or CONFIG_ABI_TLI_OPTMGMT */
			fput(filep);
			return TNOTSUPPORT;
#endif /* CONFIG_ABI_XTI_OPTMGMT or CONFIG_ABI_TLI_OPTMGMT */
		}

		case 4: /* TI_GETMYNAME */
		case 5: /* TI_SETPEERNAME */
		case 6: /* TI_GETMYNAME */
		case 7: /* TI_SETPEERNAME */
	}

#ifdef CONFIG_ABI_TRACE
	if (ibcs_trace & TRACE_STREAMS)
		printk(KERN_ERR "%d iBCS: STREAMS timod op %d not supported\n",
			current->pid, func);
#endif
	fput(filep);
	return TNOTSUPPORT;
}


int svr4_ioctl_sockmod(int fd, unsigned int func, void *arg)
{
	struct file *filep;
	struct inode *ino;
	int error;

	filep = fget(fd);
	if (!filep)
		return TBADF;
	ino = filep->f_dentry->d_inode;

	if (MAJOR(ino->i_rdev) == SOCKSYS_MAJOR) {
		error = abi_socksys_fd_init(fd, 0, NULL, NULL);
		if (error < 0)
			return error;
		fput(filep);
		filep = fget(fd);
		if (!filep)
			return TBADF;
		ino = filep->f_dentry->d_inode;
	}

	switch (func) {
#ifdef __sparc__
            	case 110: { /* SI_GETUDATA -- Solaris */
			struct {
				int tidusize, addrsize, optsize, etsdusize;
				int servtype, so_state, so_options;
			        int tsdusize;
			    
			        /* Socket parameters */
			        int family, type, protocol;
			} *it = arg;

#ifdef CONFIG_ABI_TRACE
			if (ibcs_trace & TRACE_STREAMS)
				printk(KERN_DEBUG "%d iBCS: %u new_getudata\n",
					current->pid, fd);
#endif
			error = verify_area(VERIFY_WRITE, it, sizeof(*it));
			if (error) {
				fput(filep);
				return (-error << 8) | TSYSERR;
			}

			__put_user(16384, &it->tidusize);
			__put_user(sizeof(struct sockaddr), &it->addrsize);
			__put_user(-1, &it->optsize);
			__put_user(0, &it->so_state);
			__put_user(0, &it->so_options);
			__put_user(16384, &it->tsdusize);
			
			switch (ino->u.socket_i.type) {
				case SOCK_STREAM:
					__put_user(1, &it->etsdusize);
					__put_user(2, &it->servtype);
					break;
				default:
					__put_user(-2, &it->etsdusize);
					__put_user(3, &it->servtype);
					break;
			}
			__put_user (ino->u.socket_i.ops->family, &it->family);
			__put_user (ino->u.socket_i.type, &it->type);
			__put_user (ino->u.socket_i.ops->family, &it->protocol);
			fput(filep);
			return 0;
		}
		    
#endif
		case 101: { /* SI_GETUDATA */
			struct {
				int tidusize, addrsize, optsize, etsdusize;
				int servtype, so_state, so_options;
#ifdef __sparc__
			        int tsdusize;
#endif
			} *it = arg;

#ifdef CONFIG_ABI_TRACE
			if (ibcs_trace & TRACE_STREAMS)
				printk(KERN_DEBUG "%d iBCS: %u getudata\n",
					current->pid, fd);
#endif
			error = verify_area(VERIFY_WRITE, it, sizeof(*it));
			if (error) {
				fput(filep);
				return (-error << 8) | TSYSERR;
			}

			__put_user(16384, &it->tidusize);
			__put_user(sizeof(struct sockaddr), &it->addrsize);
			__put_user(-1, &it->optsize);
			__put_user(0, &it->so_state);
			__put_user(0, &it->so_options);

#ifdef __sparc__
			__put_user(16384, &it->tsdusize);
#endif
			switch (ino->u.socket_i.type) {
				case SOCK_STREAM:
					__put_user(1, &it->etsdusize);
					__put_user(2, &it->servtype);
					break;
				default:
					__put_user(-2, &it->etsdusize);
					__put_user(3, &it->servtype);
					break;
			}
			fput(filep);
			return 0;
		}

		case 102: /* SI_SHUTDOWN */
		case 103: /* SI_LISTEN */
		case 104: /* SI_SETMYNAME */
		case 105: /* SI_SETPEERNAME */
		case 106: /* SI_GETINTRANSIT */
		case 107: /* SI_TCL_LINK */
		case 108: /* SI_TCL_UNLINK */
	}

#ifdef CONFIG_ABI_TRACE
	if (ibcs_trace & TRACE_STREAMS)
		printk(KERN_ERR "%d iBCS: STREAMS sockmod op %d not supported\n",
			current->pid, func);
#endif
	fput(filep);
	return TNOTSUPPORT;
}
EXPORT_SYMBOL(svr4_ioctl_sockmod);
#endif /* CONFIG_ABI_XTI */


#if defined(CONFIG_ABI_XTI) || defined(CONFIG_ABI_IBCS_SPX)

int timod_getmsg(int fd, struct inode *ino, int is_pmsg, struct pt_regs *regs)
{
	struct strbuf *ctlptr, *datptr;
	int *flags_p, flags, *band_p;
	int error;
	struct strbuf ctl, dat;
	struct file *filep;

	ctlptr = get_syscall_parameter (regs, 1);
	datptr = get_syscall_parameter (regs, 2);
	if (!is_pmsg) {
		flags_p = (int *)get_syscall_parameter (regs, 3);
	} else {
		band_p = (int *)get_syscall_parameter (regs, 3);
		flags_p = (int *)get_syscall_parameter (regs, 4);
		error = verify_area(VERIFY_WRITE, band_p, sizeof(int));
		if (error)
			return error;
	}

	error = verify_area(VERIFY_WRITE, flags_p, sizeof(int));
	if (error)
		return error;

	if (ctlptr) {
		error = verify_area(VERIFY_WRITE, ctlptr, sizeof(ctl));
		if (error)
			return error;
		__copy_from_user(&ctl, ctlptr, sizeof(ctl));
		__put_user(-1, &ctlptr->len);
	} else {
		ctl.maxlen = -1;
	}

	if (datptr) {
		error = verify_area(VERIFY_WRITE, datptr, sizeof(dat));
		if (error)
			return error;
		__copy_from_user(&dat, datptr, sizeof(dat));
		__put_user(-1, &datptr->len);
	} else {
		dat.maxlen = -1;
	}

	error = verify_area(VERIFY_WRITE, flags_p, sizeof(int));
	if (error)
		return error;
	__get_user(flags, flags_p);

#ifdef CONFIG_ABI_IBCS_SPX
	if (MAJOR(ino->i_rdev) == SOCKSYS_MAJOR && MINOR(ino->i_rdev) == 1) {
#ifdef CONFIG_ABI_TRACE
		if (ibcs_trace & TRACE_STREAMS)
			printk(KERN_DEBUG
				"%d iBCS: %d getmsg offers descriptor %d\n",
				current->pid, fd, fd);
#endif
		__put_user((unsigned long)fd, (unsigned long *)ctl.buf);
		__put_user(4, &ctlptr->len);
		return 0;
	}
#endif /* CONFIG_ABI_IBCS_SPX */

#ifdef CONFIG_ABI_XTI
	if (flags != 0 && flags != MSG_HIPRI && flags != MSG_ANY
	&& flags != MSG_BAND) {
#ifdef CONFIG_ABI_TRACE
		if (ibcs_trace & TRACE_STREAMS)
			printk(KERN_DEBUG
				"%d iBCS: %d getmsg flags value bad (%d)\n",
				current->pid, fd, flags);
#endif
		return -EINVAL;
	}

	filep = fget(fd);
	error = do_getmsg(fd, regs,
			ctl.buf, ctl.maxlen, &ctlptr->len,
			dat.buf, dat.maxlen, &datptr->len,
			&flags);
	fput(filep);
	if (error >= 0)
		put_user(flags, flags_p);
	return error;
#else /* CONFIG_ABI_XTI */
	return -EINVAL;
#endif /* CONFIG_ABI_XTI */
}


int timod_putmsg(int fd, struct inode *ino, int is_pmsg, struct pt_regs *regs)
{
	struct strbuf *ctlptr, *datptr;
	int flags, error, band;
	struct strbuf ctl, dat;

	ctlptr = (struct strbuf *)get_syscall_parameter (regs, 1);
	datptr = (struct strbuf *)get_syscall_parameter (regs, 2);
	if (!is_pmsg) {
		flags = (int)get_syscall_parameter (regs, 3);
	} else {
		band = (int)get_syscall_parameter (regs, 3);
		flags = (int)get_syscall_parameter (regs, 4);
	}

	if (ctlptr) {
		error = copy_from_user(&ctl, ctlptr, sizeof(ctl));
		if (error)
			return -EFAULT;
		if (ctl.len < 0 && flags)
			return -EINVAL;
	} else {
		ctl.len = 0;
		ctl.buf = NULL;
	}

	if (datptr) {
		error = copy_from_user(&dat, datptr, sizeof(dat));
		if (error)
			return -EFAULT;
	} else {
		dat.len = 0;
		dat.buf = NULL;
	}

#ifdef CONFIG_ABI_IBCS_SPX
	if (MAJOR(ino->i_rdev) == SOCKSYS_MAJOR && MINOR(ino->i_rdev) == 1) {
		unsigned int newfd;

		if (ctl.len != 4)
			return -EIO;

		get_user(newfd, (unsigned int *)ctl.buf);
#ifdef CONFIG_ABI_TRACE
		if (ibcs_trace & TRACE_STREAMS)
			printk(KERN_DEBUG
				"%d iBCS: %d putmsg dups descriptor %d\n",
				current->pid, fd, newfd);
#endif
		error = SYS(dup2)(newfd, fd);
		if (error < 0)
			return error;

		return 0;
	}
#endif /* CONFIG_ABI_IBCS_SPX */

#ifdef CONFIG_ABI_XTI
	return do_putmsg(fd, regs, ctl.buf, ctl.len,
			dat.buf, dat.len, flags);
#else /* CONFIG_ABI_XTI */
	return -EINVAL;
#endif /* CONFIG_ABI_XTI */
}

int stream_fdinsert(struct pt_regs *regs, int fd, struct strfdinsert *arg)
{
	struct strfdinsert sfd;
	int error;

	error = copy_from_user(&sfd, arg, sizeof(sfd));
	if (error)
		return -EFAULT;

#ifdef CONFIG_ABI_TRACE
	if (ibcs_trace & TRACE_STREAMS)
		printk(KERN_DEBUG "%d iBCS: %u fdinsert: flags=%ld, fildes=%u, offset=%d\n",
			current->pid, fd, sfd.flags, sfd.fildes, sfd.offset);
#endif

	return do_putmsg(fd, regs, sfd.ctlbuf.buf, sfd.ctlbuf.len,
			sfd.datbuf.buf, sfd.datbuf.len, sfd.fildes);
}

#endif /* defined(CONFIG_ABI_XTI) || defined(CONFIG_ABI_IBCS_SPX) */
