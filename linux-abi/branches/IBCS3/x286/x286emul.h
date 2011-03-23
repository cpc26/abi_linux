#ifndef _X286EMUL_H_
#define _X286EMUL_H_

extern unsigned long init_entry, init_cs, init_ds, limit_stk, init_stk, init_ss;
extern int xerrno;

extern struct xexec *xexec;
extern struct xext *xext;

extern unsigned long SEGV_STACK;
extern int LDATA,LTEXT,V3LOCK;

int verify_area(int , unsigned long , unsigned long , unsigned long ); 

#define VERIFY_WRITE 1
#define VERIFY_READ 2
#define VERIFY_EXEC 3
#define check_addr(a,b,c,d) if (verify_area(a,b,c,d)) return -1

#endif /* _X286EMUL_H_ */
