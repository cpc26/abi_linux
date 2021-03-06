/*
 *  linux/abi/hrtsys.c
 *
 *  Copyright (C) 1994 Eric Youngdale.
 *
 * The hrtsys interface is used by SVr4, and is effectively a way of doing
 * itimer.  I do not know why this is used instead of the regular itimer
 * stuff, but it appears to be related to bsd programs/functionality.
 */

#include <linux/config.h>
#define __NO_VERSION__
#include <linux/module.h>
#include <linux/version.h>

#include <asm/uaccess.h>

#include <linux/ptrace.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/string.h>

#include <abi/abi.h>

#ifdef CONFIG_ABI_TRACE
#include <abi/trace.h>
#endif


struct hrt_time_t {
  unsigned long secs;
  unsigned long sub_sec; /* Less than one second. */
  unsigned long resolution; /* Resolution of timer */
};

struct hrtcmd{
  int cmd;
  int clk;
  struct hrt_time_t interval;
  struct hrt_time_t tod;
  int flags;
  int error;
  int reserved[3];
};

static int ibcs_hrtcntl(struct pt_regs * regs) {
  unsigned int param[4];
  struct timeval * tv;
  int i, error;

  for(i=0; i<4; i++)
    param[i] = get_syscall_parameter (regs, 1+i);

  if(param[0] != 1 || param[1] != 1 || param[2] != 0) return -EINVAL;

  tv = (struct timeval *) param[3];

#ifdef CONFIG_ABI_TRACE
  if ((ibcs_trace & TRACE_API) || ibcs_func_p->trace) {
    printk(KERN_DEBUG "iBCS: hrtcntl(0x%lx)\n", (unsigned long)tv);
  }
#endif

  error = verify_area(VERIFY_WRITE, (char *) tv,sizeof *tv);
  if (error)
    return error;

  return SYS(gettimeofday)(tv, NULL);
}

static int ibcs_hrtalarm(struct pt_regs * regs) {
  struct itimerval get_buffer;
  struct hrtcmd * hcmd;
  int i, error, cmd, retval, which;
  mm_segment_t old_fs = get_fs();

  i = get_syscall_parameter (regs, 2);
  if(i != 1) return -EINVAL;

  hcmd = (struct hrtcmd *) get_syscall_parameter (regs, 1);

  error = verify_area(VERIFY_WRITE, (char *) hcmd,sizeof *hcmd);
  if (error)
    return error;

  get_user(cmd, ((unsigned long *) hcmd));

  /* Now figure out which clock we want to fiddle with */
  get_user(which, ((unsigned long *) hcmd)+1);

#ifdef CONFIG_ABI_TRACE
  if ((ibcs_trace & TRACE_API) || ibcs_func_p->trace) {
   printk(KERN_DEBUG "iBCS: %d hrtalarm(0x%lx %d)\n",
	 current->pid, (unsigned long)cmd, which);
  }
#endif

  switch(which){
  case 4:
    which = 2;
    break;
  case 2:
    which = 1;
    break;
  case 1:
    which = 0;
    break;
  default:
    return -EINVAL;
  };

  switch(cmd) {
  case 0xc:
    if(({long r; get_user(r, ((unsigned long *) hcmd)+4); r;}) != 1000000) return -EINVAL;
    copy_from_user(&get_buffer.it_value, ((unsigned long *) hcmd)+2,
		   sizeof(struct timeval));
    memset(&get_buffer.it_interval, 0, sizeof(struct timeval));
    set_fs(get_ds());
    retval = SYS(setitimer)(which, &get_buffer, NULL);
    set_fs(old_fs);
    break;
  case 0xd:
    set_fs(get_ds());
    retval = SYS(getitimer)(which, &get_buffer);
    set_fs(old_fs);
#ifdef CONFIG_ABI_TRACE
    if ((ibcs_trace & TRACE_API) || ibcs_func_p->trace) {
      printk(KERN_DEBUG "iBCS: hrtalarm(d %lx) %x %x %x %x\n",
	   (unsigned long)hcmd,
	   get_buffer.it_interval.tv_sec,
	   get_buffer.it_interval.tv_usec,
	   get_buffer.it_value.tv_sec,
	   get_buffer.it_value.tv_usec);
    }
#endif
    put_user(1000000, &hcmd->interval.resolution);
    copy_to_user(((unsigned long *) hcmd)+2, &get_buffer.it_interval,
		   sizeof(get_buffer));
    retval = 1;
    break;
  case 0xf:
    if(({long r; get_user(r, ((unsigned long *) hcmd)+4); r;}) != 1000000) return -EINVAL;
    if(({long r; get_user(r, ((unsigned long *) hcmd)+7); r;}) != 1000000) return -EINVAL;
    copy_from_user(&get_buffer.it_value, &hcmd->tod,
		   sizeof(struct timeval));
    copy_from_user(&get_buffer.it_interval, &hcmd->interval,
		   sizeof(struct timeval));
    set_fs(get_ds());
    retval = SYS(setitimer)(which, &get_buffer, NULL);
    set_fs(old_fs);
    break;
  case 0x10:
    memset(&get_buffer, 0, sizeof(get_buffer));
    set_fs(get_ds());
    retval = SYS(setitimer)(which, &get_buffer, NULL);
    set_fs(old_fs);
    break;
  default:
    retval = -EINVAL;
  };
  return retval;
}

int svr4_hrtsys(struct pt_regs * regs)
{
  int func, retval;

  func  = get_syscall_parameter (regs, 0);

#ifdef CONFIG_ABI_TRACE
  if ((ibcs_trace & TRACE_API) || ibcs_func_p->trace) {
    printk(KERN_DEBUG "iBCS: hrtsys(%d)\n", func);
  }
#endif


  switch(func){
  case 0:
    retval = ibcs_hrtcntl(regs);
    break;
  case 1:
    retval = ibcs_hrtalarm(regs);
    break;
  case 2:
  case 3:
  default:
    retval = -EINVAL;
    break;
  }
  return retval;
}

EXPORT_SYMBOL(svr4_hrtsys);
