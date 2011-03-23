/*
 * Copyright (c) 2008 Christian Lademann, ZLS Software GmbH <cal@zls.de>
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

//#ident "%W% %G%"

#include "../include/util/i386_std.h"
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/file.h>
#if _KSL > 25
#include <linux/fdtable.h>
#endif
#include <linux/string.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/vmalloc.h>
#include <linux/proc_fs.h>
#include <linux/version.h>
#include <asm/uaccess.h>

#include "../include/svr4/types.h"
#include "../include/svr4/stat.h"

#include "../include/util/stat.h"
#include "../include/util/trace.h"

int short_inode_mapping = 0;		/* enable short inode mapping */
int short_inode_map_size = -1;	/* size of short inode map */

MODULE_PARM_DESC(short_inode_mapping, "Map inode numbers to fit in 2 bytes (default=0)");
module_param(short_inode_mapping, int, 0);

MODULE_PARM_DESC(short_inode_map_size, "Size of map for inode numbers (default=65533)");
module_param(short_inode_map_size, int, 0);


/*
 * SHORT INODE MAPPING:
 * Keep a list of mappings from 32-bit inode numbers to 16-bit inode numbers.
 */

struct short_inode_map_element {
	ino_t	ino;
	u_short	lru_prev;
	u_short	lru_next;
	u_short	hash_first;
	u_short	hash_prev;
	u_short	hash_next;
} short_inode_map_element;

typedef struct short_inode_map_element short_inode_map_element_t;


struct short_inode_map_check_results {
	int		map_size;
	int		nr_entries;
	int		nr_used;
	int		nr_hashes;
	int		ok_hashes;
	int		ok_lru;
} short_inode_map_check_results;

typedef struct short_inode_map_check_results short_inode_map_check_results_t;


#define SHORT_INODE_MAP_MINSIZE	0xfd
#define SHORT_INODE_MAP_MAXSIZE	0xfffd
#define SHORT_INODE_MAP_NULL	0xffff

static short_inode_map_element_t *SHORT_INODE_MAP = NULL;
static int SHORT_INODE_MAP_SIZE = SHORT_INODE_MAP_MAXSIZE;
static int SHORT_INODE_MAP_INITIALIZED = 0;
static u_short SHORT_INODE_MAP_OLDEST = 0;

static spinlock_t SHORT_INODE_MAP_LOCK = SPIN_LOCK_UNLOCKED; 

void short_inode_map_dump(void);
int short_inode_map_initialize(void);
u_short short_inode_map_hashval(ino_t);
void short_inode_map_hash(u_short, ino_t);
void short_inode_map_use(u_short);
ino_t short_inode_map(ino_t);
int short_inode_map_check_idx(u_short);
int short_inode_map_check_lru(void);
int short_inode_map_check_hash(u_short);
int short_inode_map_check(short_inode_map_check_results_t *);

#if defined(CONFIG_PROC_FS) && defined(CONFIG_ABI_PROC)
static int short_inode_mapping_proc_info(char *, char **, off_t, int, int *, void *);
static int short_inode_mapping_proc_write(struct file * file, const char * buf, unsigned long length, void *data);
#endif


/*
 * Allocate SHORT_INODE_MAP and initialize the linked lists
 */
int
short_inode_map_initialize() {
	int	i;

	spin_lock(&SHORT_INODE_MAP_LOCK);

	if(! SHORT_INODE_MAP_INITIALIZED) {
		int	needed_space = -1;

		if(SHORT_INODE_MAP_SIZE < SHORT_INODE_MAP_MINSIZE)
			SHORT_INODE_MAP_SIZE = SHORT_INODE_MAP_MINSIZE;
 		else if(SHORT_INODE_MAP_SIZE > SHORT_INODE_MAP_MAXSIZE)
			SHORT_INODE_MAP_SIZE = SHORT_INODE_MAP_MAXSIZE;

		needed_space = SHORT_INODE_MAP_SIZE * sizeof(short_inode_map_element_t);

		if((SHORT_INODE_MAP = (short_inode_map_element_t*)vmalloc(needed_space)) == NULL) {
			__abi_trace("abi-util: could not allocate %d bytes for SHORT_INODE_MAP\n", needed_space);
			return -1;
		}

		for(i = 0; i < SHORT_INODE_MAP_SIZE; i++) {
			SHORT_INODE_MAP[i].ino = (ino_t)0;
			SHORT_INODE_MAP[i].hash_first = SHORT_INODE_MAP_NULL;
			SHORT_INODE_MAP[i].hash_prev = i;
			SHORT_INODE_MAP[i].hash_next = i;

			if(i == 0) {
				SHORT_INODE_MAP[i].lru_prev = SHORT_INODE_MAP_SIZE - 1;
				SHORT_INODE_MAP[i].lru_next = i + 1;
			} else if(i == SHORT_INODE_MAP_SIZE - 1) {
				SHORT_INODE_MAP[i].lru_prev = i - 1;
				SHORT_INODE_MAP[i].lru_next = 0;
			} else {
				SHORT_INODE_MAP[i].lru_prev = i - 1;
				SHORT_INODE_MAP[i].lru_next = i + 1;
			}
		}

		SHORT_INODE_MAP_OLDEST = 0;

		SHORT_INODE_MAP_INITIALIZED = 1;
	}

	spin_unlock(&SHORT_INODE_MAP_LOCK);

	return 0;
}


/*
 * Dump the SHORT_INODE_MAP. Insert waits to allow
 * klogd to catch the complete dump.
 */
void
short_inode_map_dump() {
	int	i;

	if(! short_inode_mapping || ! SHORT_INODE_MAP_INITIALIZED)
		return;

	spin_lock(&SHORT_INODE_MAP_LOCK);

	__abi_trace("SHORT_INODE_MAP dump: OLDEST=%x\n", SHORT_INODE_MAP_OLDEST);

	for(i = 0; i < SHORT_INODE_MAP_SIZE; i++) {
		__abi_trace(
			"[%4x] %04x:%08lx %4x %4x %4x %4x %4x\n",
			i,
			0,
			(u_long)(SHORT_INODE_MAP[i].ino),
			SHORT_INODE_MAP[i].hash_first,
			SHORT_INODE_MAP[i].hash_prev,
			SHORT_INODE_MAP[i].hash_next,
			SHORT_INODE_MAP[i].lru_prev,
			SHORT_INODE_MAP[i].lru_next
		);

		if(i % 100 == 0) {
			/* Give klogd some time to consume messages */
			set_current_state(TASK_INTERRUPTIBLE);
			schedule_timeout(HZ / 20);
		}
	}

	__abi_trace("SHORT_INODE_MAP dumped.\n");

	spin_unlock(&SHORT_INODE_MAP_LOCK);
}


/*
 * Determine the 16-bit hash value for an inode number.
 */
u_short
short_inode_map_hashval(ino_t ino) {
	u_short	h;

	if((h = (u_short)((u_long)ino & 0xffff)) >= SHORT_INODE_MAP_SIZE)
		if((h = (u_short)(((u_long)ino & 0xffff0000) >> 16)) >= SHORT_INODE_MAP_SIZE)
			h = (u_short)(SHORT_INODE_MAP_SIZE - 1);

	/*
     * short_inode_map() returns (index + 2). If we 'wrap' the calculated
     * hashvalue by -2 here, we eventually can see the same inode numbers
     * as linux, as long as they fit into 16 bit.
     */
	if(h >= 2)
		return h - 2;
	else
		return (u_short)(SHORT_INODE_MAP_SIZE - 1 - h);
}


/*
 * Insert entry i in the linked list of entries with the same
 * hash value, possibly removing in from an old list first.
 */
void
short_inode_map_hash(u_short i, ino_t oino) {
	u_short	n = 0;
	u_short	p = 0;
	u_short	f = 0;
	u_short	h = short_inode_map_hashval(SHORT_INODE_MAP[i].ino);
	u_short	oh = short_inode_map_hashval(oino);

	/* remove element from old hash list */
	p = SHORT_INODE_MAP[i].hash_prev;
	n = SHORT_INODE_MAP[i].hash_next;

	SHORT_INODE_MAP[n].hash_prev = p;
	SHORT_INODE_MAP[p].hash_next = n;

	if(SHORT_INODE_MAP[oh].hash_first == i)
		SHORT_INODE_MAP[oh].hash_first = (n == i ? SHORT_INODE_MAP_NULL : n);

	/* add element to new hash list */
	if((f = SHORT_INODE_MAP[h].hash_first) == SHORT_INODE_MAP_NULL) {
		SHORT_INODE_MAP[i].hash_prev = i;
		SHORT_INODE_MAP[i].hash_next = i;
	} else {
		p = SHORT_INODE_MAP[f].hash_prev;

		SHORT_INODE_MAP[f].hash_prev = i;
		SHORT_INODE_MAP[p].hash_next = i;

		SHORT_INODE_MAP[i].hash_prev = p;
		SHORT_INODE_MAP[i].hash_next = f;
	}

	SHORT_INODE_MAP[h].hash_first = i;
}


/*
 * Move an entry to the end of the lru (Least Recently Used) list.
 */
void
short_inode_map_use(u_short i) {
	u_short	n = 0;
	u_short	p = 0;

	if(i == SHORT_INODE_MAP_OLDEST) {
		SHORT_INODE_MAP_OLDEST = SHORT_INODE_MAP[i].lru_next;
	} else if(i != SHORT_INODE_MAP[SHORT_INODE_MAP_OLDEST].lru_prev) {
		p = SHORT_INODE_MAP[i].lru_prev;
		n = SHORT_INODE_MAP[i].lru_next;

		SHORT_INODE_MAP[n].lru_prev = p;
		SHORT_INODE_MAP[p].lru_next = n;

		p = SHORT_INODE_MAP[SHORT_INODE_MAP_OLDEST].lru_prev;
		n = SHORT_INODE_MAP_OLDEST;

		SHORT_INODE_MAP[i].lru_prev = p;
		SHORT_INODE_MAP[i].lru_next = n;

		SHORT_INODE_MAP[p].lru_next = i;
		SHORT_INODE_MAP[n].lru_prev = i;
	}
}


/*
 * If short_inode_mapping is enabled, find an mapping from the
 * 32-bit inode number ino to a 16-bit one or create a new mapping.
 * Inode numbers 0 and 1 (32-bit) are always returned as 0 and 1
 * and never as the result of a mapping.
 */
ino_t
short_inode_map(ino_t ino) {
	u_long	li = 0;
	u_short	si = 0;
	u_short	hf = SHORT_INODE_MAP_NULL;
	int	i = 0;
	int	simp = -1;

	if(! short_inode_mapping) {
		if((u_long)ino & 0xffff)
			return ino;

		return 0xfffffffe;
	}

	li = (u_long)ino;

	if(li == 0 || li == 1) {
		abi_trace(ABI_TRACE_API, "short_inode_map(0x%x) --> 0x%x (%d)\n", (u_int)ino, 0, __LINE__);
		return ino;
	}

	spin_lock(&SHORT_INODE_MAP_LOCK);

	simp = -1;

	si = short_inode_map_hashval(ino);
	hf = SHORT_INODE_MAP[si].hash_first;

	if(hf == SHORT_INODE_MAP_NULL) {
		if(SHORT_INODE_MAP[si].ino == (ino_t)0) {
 			SHORT_INODE_MAP[si].ino = ino;
 			SHORT_INODE_MAP[si].hash_first = si;
 			SHORT_INODE_MAP[si].hash_next = si;
 			SHORT_INODE_MAP[si].hash_prev = si;
			simp = si;
			// abi_trace(ABI_TRACE_API, "short_inode_map(0x%x) --> 0x%x (%d) (ino into first)\n", (u_int)ino, si, __LINE__);
		}
	} else {
		i = hf;

		do {
			if(SHORT_INODE_MAP[i].ino == ino)
				simp = i;
			else
				i = SHORT_INODE_MAP[i].hash_next;
		} while(simp < 0 && i != hf);
	}

	if(simp >= 0) {
		// abi_trace(ABI_TRACE_API, "short_inode_map(0x%x) --> 0x%x (%d)\n", (u_int)ino, simp, __LINE__);
  		short_inode_map_use(simp);
	} else {
		ino_t	oino = (ino_t)0;

		simp = SHORT_INODE_MAP_OLDEST;
		SHORT_INODE_MAP_OLDEST = SHORT_INODE_MAP[SHORT_INODE_MAP_OLDEST].lru_next;

		oino = SHORT_INODE_MAP[simp].ino;
		SHORT_INODE_MAP[simp].ino = ino;
	
  		short_inode_map_hash(simp, oino);
		// abi_trace(ABI_TRACE_API, "short_inode_map(0x%x) --> 0x%x (%d) (ino into hash)\n", (u_int)ino, simp, __LINE__);
	}

	spin_unlock(&SHORT_INODE_MAP_LOCK);

	abi_trace(ABI_TRACE_API, "short_inode_map(0x%x) --> 0x%x (%d), O=%d->%d->%d\n", (u_int)ino, simp + 2, __LINE__, SHORT_INODE_MAP[SHORT_INODE_MAP_OLDEST].lru_prev, SHORT_INODE_MAP_OLDEST, SHORT_INODE_MAP[SHORT_INODE_MAP_OLDEST].lru_next);

	return (ino_t)(simp + 2);
}


int
short_inode_map_check_idx(u_short i) {
	if(i >= SHORT_INODE_MAP_SIZE)
		return(0);

	return(1);
}


int
short_inode_map_check_hash(u_short idx) {
	u_short	f = 0;
	u_short	i = 0;
	u_short	l = 0;
	int	m = 0;

	if(! short_inode_map_check_idx(idx)) {
		__abi_trace("short_inode_map_check_hash(), line %d: invalid index (0x%x)\n", __LINE__, idx);
		return 0;
	}

	f = SHORT_INODE_MAP[idx].hash_first;

	if(! short_inode_map_check_idx(f)) {
		__abi_trace("short_inode_map_check_hash(), line %d: invalid hash_first (0x%x)\n", __LINE__, f);
		return 0;
	}

	i = f;
	l = SHORT_INODE_MAP[i].hash_prev; /* HACK */

	m = 0;

	do {
		u_short	h = 0;

		if(! short_inode_map_check_idx(i)) {
			__abi_trace("short_inode_map_check_hash(), line %d: invalid index in hash list (0x%x)\n", __LINE__, i);
			return 0;
		}

		if((h = short_inode_map_hashval(SHORT_INODE_MAP[i].ino)) != idx) {
			__abi_trace("short_inode_map_check_hash(), line %d, index 0x%x: invalid ino in hash list (0x%x != 0x%x)\n", __LINE__, i, h, idx);
			return 0;
		}

		if(SHORT_INODE_MAP[i].hash_prev != l) {
			__abi_trace("short_inode_map_check_hash(), line %d, index 0x%x: invalid backlink in hash list (0x%x != 0x%x)\n", __LINE__, i, SHORT_INODE_MAP[i].hash_prev, l);
			return 0;
		}

		l = i;
		i = SHORT_INODE_MAP[i].hash_next;

		m++;
	} while(i != f && m < SHORT_INODE_MAP_SIZE);

	return 1;
}


int
short_inode_map_check_lru() {
	u_short	i = 0;
	u_short	l = 0;
	int	m = 0;

	if(! short_inode_map_check_idx(SHORT_INODE_MAP_OLDEST)) {
		__abi_trace("short_inode_map_check_lru(), line %d: invalid head index (0x%x)\n", __LINE__, SHORT_INODE_MAP_OLDEST);
		return 0;
	}

	i = SHORT_INODE_MAP_OLDEST;
	l = SHORT_INODE_MAP[i].lru_prev; /* HACK */

	m = 0;

	do {
		if(! short_inode_map_check_idx(i)) {
			__abi_trace("short_inode_map_check_lru(), line %d: invalid index (0x%x)\n", __LINE__, i);
			return 0;
		}

		if(SHORT_INODE_MAP[i].lru_prev != l) {
			__abi_trace("short_inode_map_check_lru(), line %d, index %d: invalid backlink (0x%x != 0x%x)\n", __LINE__, i, SHORT_INODE_MAP[i].lru_prev, l);
			return 0;
		}

		l = i;
		i = SHORT_INODE_MAP[i].lru_next;

		m++;
	} while(i != SHORT_INODE_MAP_OLDEST && m < SHORT_INODE_MAP_SIZE + 1);

	return(m);
}


int
short_inode_map_check(short_inode_map_check_results_t* results) {
	int	i;

	if(results) {
		results->map_size = 0;
		results->nr_entries = 0;
		results->nr_used = 0;
		results->nr_hashes = 0;
		results->ok_hashes = 0;
		results->ok_lru = 0;
	}

	if(! short_inode_mapping || ! results)
		return 0;

	spin_lock(&SHORT_INODE_MAP_LOCK);

	results->map_size = sizeof(short_inode_map_element_t) * SHORT_INODE_MAP_SIZE;
	results->nr_entries = SHORT_INODE_MAP_SIZE;

	for(i = 0; i < SHORT_INODE_MAP_SIZE; i++) {
		if(SHORT_INODE_MAP[i].ino != (ino_t)0)
			results->nr_used++;

		if(SHORT_INODE_MAP[i].hash_first != SHORT_INODE_MAP_NULL) {
			results->nr_hashes++;

			if(short_inode_map_check_hash(i))
				results->ok_hashes++;
		}
	}

	results->ok_lru = short_inode_map_check_lru();

	spin_unlock(&SHORT_INODE_MAP_LOCK);

	return 1;
}


#if defined(CONFIG_PROC_FS) && defined(CONFIG_ABI_PROC)
static int
short_inode_mapping_proc_info(char *buffer, char **start, off_t offset, int length, int *eof, void *data) {
	short_inode_map_check_results_t	results;
	int len = 0;
	off_t begin = 0;
	off_t pos = 0;

	short_inode_map_check(&results);

	len += sprintf(buffer + len, "map size:          %d\n", results.map_size);
	len += sprintf(buffer + len, "total elements:    %d\n", results.nr_entries);
	len += sprintf(buffer + len, "used elements:     %d\n", results.nr_used);
	len += sprintf(buffer + len, "total hash queues: %d\n", results.nr_hashes);
	len += sprintf(buffer + len, "valid hash queues: %d (%s)\n", results.ok_hashes, (results.nr_hashes == results.ok_hashes ? "OK" : "ERROR"));
	len += sprintf(buffer + len, "valid lru entries: %d (%s)\n", results.ok_lru, (results.nr_entries == results.ok_lru ? "OK" : "ERROR"));

	pos = begin + len;

	*start = buffer + (offset - begin);	/* Start of wanted data */
	len -= (offset - begin);	/* Start slop */
	if (len > length)
		len = length;	/* Ending slop */
	*eof = (len <= 0 ? 1 : 0);
	return (len);
}


static int
short_inode_mapping_proc_write(struct file * file, const char * buf, unsigned long length, void *data) {
	char * buffer;
	int err;

	if (!buf || length>PAGE_SIZE)
		return -EINVAL;

	if (!(buffer = (char *) __get_free_page(GFP_KERNEL)))
		return -ENOMEM;
	if(copy_from_user(buffer, buf, length))
	{
		err =-EFAULT;
		goto out;
	}

	err = -EINVAL;

	if (length < PAGE_SIZE)
		buffer[length] = '\0';
	else if (buffer[PAGE_SIZE-1])
		goto out;


	err = length;

	if(length >= 7 && strncmp(buffer, "dumpmap", 7) == 0)
		short_inode_map_dump();

out:
	
	free_page((unsigned long) buffer);
	return err;
}
#endif


int
abi_shinomap_init(void)
{
	SHORT_INODE_MAP_LOCK = SPIN_LOCK_UNLOCKED; 
	SHORT_INODE_MAP_INITIALIZED = 0;

	SHORT_INODE_MAP_SIZE = SHORT_INODE_MAP_MAXSIZE;

	if(short_inode_map_size >= 0) {
		if(short_inode_map_size == 0)
			short_inode_mapping = 0;
		else
			SHORT_INODE_MAP_SIZE = short_inode_map_size;
	}

	if(short_inode_mapping)
		if(short_inode_map_initialize())
			return -ENOMEM;

#if defined(CONFIG_PROC_FS) && defined(CONFIG_ABI_PROC)
	{
		struct proc_dir_entry *pe;

		if(! (pe = create_proc_read_entry("abi/short_inode_mapping", S_IFREG | S_IRUGO | S_IWUSR, NULL, short_inode_mapping_proc_info, NULL))) {
			printk(KERN_ERR "cannot init /proc/abi/short_inode_mapping\n");
			remove_proc_entry("abi/abi", NULL);
			remove_proc_entry("abi", NULL);
			return -ENOMEM;
		}

		pe->write_proc = short_inode_mapping_proc_write;
	}
#endif

	return 0;
}


void
abi_shinomap_exit(void)
{
#if defined(CONFIG_PROC_FS) && defined(CONFIG_ABI_PROC)
	remove_proc_entry("abi/short_inode_mapping", 0);
#endif

	if(short_inode_mapping) {
		if(SHORT_INODE_MAP != NULL) {
			vfree(SHORT_INODE_MAP);
			SHORT_INODE_MAP = NULL;
		}
	}
}


EXPORT_SYMBOL(short_inode_map);
