#ifndef __ABI_UW7_CONTEXT_H__
#define __ABI_UW7_CONTEXT_H__

#ifdef __KERNEL__

/* ss_size <-> ss_flags which is why we can't use native Linux stack_t :( */
typedef struct uw7_stack {
	void	*ss_sp;
	int 	ss_size;
	int	ss_flags;
} uw7_stack_t;

/* XXX more registers, please */
typedef struct uw7_mcontext {
	unsigned short gs, __gsh;
	unsigned short fs, __fsh;
	unsigned short es, __esh;
	unsigned short ds, __dsh;
} uw7_mcontext_t;

typedef struct uw7_sigset {
	unsigned int sigbits[4];
} uw7_sigset_t;

typedef struct uw7_context {
	unsigned long		uc_flags;
	struct uw7_context	*uc_link;
	uw7_sigset_t		uc_sigmask;
	uw7_stack_t		uc_stack;
	uw7_mcontext_t		uc_mcontext;
	void			*uc_pdata;
	char			uc_unused[16];
} uw7_context_t;

#define UW7_GETCONTEXT		0
#define UW7_SETCONTEXT		1
#define UW7_GETXCONTEXT		2


/* context.c */
extern int uw7_context(struct pt_regs * regs);
extern int uw7_sigaltstack(const uw7_stack_t *ss, uw7_stack_t *oss);

#endif /* __KERNEL__ */
#endif /* __ABI_UW7_CONTEXT_H__ */
