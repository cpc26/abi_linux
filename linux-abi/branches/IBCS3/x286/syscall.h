#ifndef _EMU_SYSCALL_H_
#define _EMU_SYSCALL_H_

/* From emu_memory.c */
extern int emu_stkgro(struct sigcontext *sc);
extern int emu_brkctl(struct sigcontext *sc);
extern int emu_brk(struct sigcontext *sc);

/* From emu_generic.c */
extern int emu_i_sas(struct sigcontext *sc);
extern int emu_i_ass(struct sigcontext *sc);
extern int emu_i_s(struct sigcontext *sc);
extern int emu_i_sls(struct sigcontext *sc);
extern int emu_i_ssl(struct sigcontext *sc);
/* joerg */
extern int emu_locking(struct sigcontext *sc);
/*- joerg */
extern int emu_i_a(struct sigcontext *sc);
extern int emu_i_aa(struct sigcontext *sc);
extern int emu_i_ssa(struct sigcontext *sc);
extern int emu_i_l(struct sigcontext *sc);
extern int emu_i_v(struct sigcontext *sc);

/* From emu_odd.c */
extern int emu_i_pipe(struct sigcontext *sc);
extern int emu_i_fcntl(struct sigcontext *sc);
extern int emu_i_fstat(struct sigcontext *sc);
extern int emu_i_stat(struct sigcontext *sc);
extern int emu_i_ftime(struct sigcontext *sc);
extern int emu_i_ioctl(struct sigcontext *sc);
extern int emu_i_dup(struct sigcontext *sc);
extern int emu_i_ulimit(struct sigcontext *sc);

/* From emu_signal.c */
#include "emu_signal.h"

/* From emu_exec.c */
extern int emu_exec(struct sigcontext *sc);

#endif /* _EMU_SYSCALL_H_ */
