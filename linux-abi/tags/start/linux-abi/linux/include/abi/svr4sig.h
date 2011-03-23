#ifndef __ABI_SVR4SIG_H__
#define __ABI_SVR4SIG_H__
typedef void (*svr4_sig_t)(int, void *, void *);

typedef struct {
	u32 setbits [4];
} svr4_sigset_t;


#endif
