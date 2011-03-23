/*
 *  include/abi/map.h
 *
 *  Copyright (C) 1994  Mike Jagdis (jaggy@purplet.demon.co.uk)
 *
 * $Id$
 * $Source$
 */

#ifndef __ABI_MAP_H__
#define __ABI_MAP_H__

struct map_segment {
	int start, end;
	unsigned char *map;
};


extern long map_bitvec(unsigned long vec, long map[]);
extern unsigned long map_sigvec_from_kernel(sigset_t vec, unsigned long map[]);
extern sigset_t map_sigvec_to_kernel(unsigned long vec, unsigned long map[]);
extern int map_value(struct map_segment *m, int val, int def);

#endif
