#ifdef DEBUG

#include <stdio.h>
#include <fcntl.h>
extern FILE *__dbf;
#define d_print(x...) if(__dbf) fprintf(__dbf, ##x)

#else

#define d_print(x...)
#undef DEBUG_CALL
#undef DEBUG_STACK
#undef DEBUG_ENV

#endif

#ifdef DEBUG_CALL
extern void dump_state(unsigned char *, unsigned short *, struct sigcontext *);
#else
#define dump_state(x...)
#endif
