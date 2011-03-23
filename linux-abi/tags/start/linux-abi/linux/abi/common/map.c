/*
 *  linux/abi/abi_common/map.c
 *
 *  Copyright (C) 1994  Mike Jagdis (jaggy@purplet.demon.co.uk)
 *
 * $Id$
 * $Source$
 */
#include <linux/config.h>
#define __NO_VERSION__
#include <linux/module.h>

#include <linux/version.h>

#include <linux/sched.h>
#include <linux/net.h>
#include <linux/socket.h>
#include <linux/signal.h>
#include <linux/errno.h>
#include <linux/personality.h>

#include <abi/socket.h>
#include <abi/map.h>

#ifndef NULL
#define NULL	((void *)0)
#endif


long
map_bitvec(unsigned long vec, long map[])
{
	unsigned long newvec, m;
	int i;

	newvec = 0;
	for (m=1,i=1; i<=32; m<<=1,i++)
		if ((vec & m) && map[i] != -1)
			newvec |= (1 << map[i]);

	return newvec;
}

EXPORT_SYMBOL(map_bitvec);

unsigned long
map_sigvec_from_kernel(sigset_t vec, unsigned long map[])
{
	unsigned long newvec;
	int i;

	newvec = 0;
	for (i=1; i<=32; i++) {
		if (sigismember(&vec, i) && map[i] != -1)
			newvec |= (1 << map[i]);
	}
	return newvec;
}

EXPORT_SYMBOL(map_sigvec_from_kernel);

sigset_t
map_sigvec_to_kernel(unsigned long vec, unsigned long map[])
{
	sigset_t newvec;
	unsigned long m;
	int i;

	sigemptyset(&newvec);
	for (m=1,i=1; i<=32; m<<=1,i++) {
		if ((vec & m) && map[i] != -1)
			sigaddset(&newvec, map[i]);
	}
	return newvec;
}

EXPORT_SYMBOL(map_sigvec_to_kernel);

int
map_value(struct map_segment *m, int val, int def) {
	struct map_segment *seg;

	/* If no mapping exists in this personality just return the
	 * number we were given.
	 */
	if (!m)
		return val;

	/* Search the map looking for a mapping for the given number. */
	for (seg=m; seg->start != -1; seg++) {
		if (seg->start <= val && val <= seg->end) {
			/* If the start and end are the same then this
			 * segment has one entry and the map is the value
			 * it maps to. Otherwise if we have a vector we
			 * pick out the relevant value, if we don't have
			 * a vector we give identity mapping.
			 */
			if (seg->start == seg->end)
				return (int)seg->map;
			else
				return (seg->map ? seg->map[val-seg->start] : val);
		}
	}

	/* Number isn't mapped. Returned the requested default. */
	return def;
}

EXPORT_SYMBOL(map_value);
