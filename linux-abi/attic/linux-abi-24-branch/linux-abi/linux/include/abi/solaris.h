#ifndef __IBCS_SOLX86_H__
#define __IBCS_SOLX86_H__

#include <abi/lfs.h>

typedef void svr4_ucontext_t;

int sol_llseek(struct pt_regs * regs);
int ibcs_memcntl(unsigned addr, unsigned len, int cmd, unsigned arg, 
		 int attr, int mask);




#define GETACL                  1
#define SETACL                  2
#define GETACLCNT               3

int sol_acl(char *pathp, int cmd, int nentries, void *aclbufp);

#endif /* __ IBCS_SOLX86_H__ */
