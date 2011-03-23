/*
 * compatibility wrapper for file table changes
 *
 * Copyright 2000 Manfred Spraul, based on the initial
 *      bugfix from Christian Lademann
 *
 */

#if LINUX_VERSION_CODE < 0x02020c    /* less than 2.2.12 */

#define FDS_RLIMIT		NR_OPEN

#define	FDS_TABLE_LEN		NR_OPEN
#define OPEN_FDS_ADDR		(&current->files->open_fds)
#define CLOSE_ON_EXEC_FDS_ADDR	(&current->files->close_on_exec)


#else /* from 2.2.12: dynamic file tables added */

#define FDS_RLIMIT		(current->rlim[RLIMIT_NOFILE].rlim_cur)

#define FDS_TABLE_LEN		(current->files->max_fdset)
#define OPEN_FDS_ADDR		(current->files->open_fds)
#define CLOSE_ON_EXEC_FDS_ADDR	(current->files->close_on_exec)

#endif

#define CHECK_LOCK()		do { } while(0)

/* debugging code, only usable for SMP kernels:

#define CHECK_LOCK()	if(current->lock_depth < 0) do { *(int*)0=0; } while(0)
*/
