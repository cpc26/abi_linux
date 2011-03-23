/*
 * Mostly ripped from Al Viro's stat-a-AC9-10 patch, 2001 Christoph Hellwig.
 */
/*
 * short-inode-mapping code: Copyright (c) 2005  Christian Lademann, ZLS Software GmbH.
 */

#ident "%W% %G%"

#include <linux/module.h>		/* needed to shut up modprobe */
#include <linux/fs.h>
#include <linux/stat.h>
#include <linux/sched.h>
#include <linux/file.h>
#include <linux/personality.h>
#include <linux/vmalloc.h>
#include <linux/proc_fs.h>
#include <linux/compatmac.h>

#include <abi/util/revalidate.h>
#include <abi/util/stat.h>
#include <abi/util/trace.h>


static int short_inode_mapping = 0;		/* enable short inode mapping */
static int short_inode_map_size = -1;	/* size of short inode map */

MODULE_DESCRIPTION("Linux-ABI helper routines");
MODULE_AUTHOR("Christoph Hellwig, ripped from kernel sources/patches");
MODULE_LICENSE("GPL");
MODULE_PARM(short_inode_mapping, "i");
MODULE_PARM_DESC(short_inode_mapping, "Map inode numbers to fit in 2 bytes (default=0)");
MODULE_PARM(short_inode_map_size, "i");
MODULE_PARM_DESC(short_inode_map_size, "Size of map for inode numbers (default=65533)");

#ifdef CONFIG_PROC_FS
struct proc_dir_entry *proc_abi;
static int abi_gen_proc_write(struct file * file, const char * buf, unsigned long length, void *data);
static int abi_proc_info(char *, char **, off_t, int);
static int short_inode_mapping_proc_info(char *, char **, off_t, int);
static int short_inode_mapping_proc_write(struct file * file, const char * buf, unsigned long length, void *data);
#endif


int getattr_full(struct vfsmount *mnt, struct dentry *dentry, struct kstat *stat)
{
	struct inode *inode = dentry->d_inode;
	stat->dev = kdev_t_to_nr(inode->i_dev);
	stat->ino = inode->i_ino;
	stat->mode = inode->i_mode;
	stat->nlink = inode->i_nlink;
	stat->uid = inode->i_uid;
	stat->gid = inode->i_gid;
	stat->rdev = kdev_t_to_nr(inode->i_rdev);
	stat->atime = inode->i_atime;
	stat->mtime = inode->i_mtime;
	stat->ctime = inode->i_ctime;
	stat->size = inode->i_size;
	stat->blocks = inode->i_blocks;
	stat->blksize = inode->i_blksize;
	return 0;
}

/*
 * Use minix fs values for the number of direct and indirect blocks.  The
 * count is now exact for the minix fs except that it counts zero blocks.
 * Everything is in units of BLOCK_SIZE until the assignment to
 * stat->blksize.
 */
int getattr_minix(struct vfsmount *mnt, struct dentry *dentry, struct kstat *stat)
{
	struct inode *inode = dentry->d_inode;
	unsigned int blocks, indirect;

	stat->dev = kdev_t_to_nr(inode->i_dev);
	stat->ino = inode->i_ino;
	stat->mode = inode->i_mode;
	stat->nlink = inode->i_nlink;
	stat->uid = inode->i_uid;
	stat->gid = inode->i_gid;
	stat->rdev = kdev_t_to_nr(inode->i_rdev);
	stat->atime = inode->i_atime;
	stat->mtime = inode->i_mtime;
	stat->ctime = inode->i_ctime;
	stat->size = inode->i_size;
#define D_B	7
#define I_B	(BLOCK_SIZE / sizeof(unsigned short))

	blocks = (stat->size + BLOCK_SIZE - 1) >> BLOCK_SIZE_BITS;
	if (blocks > D_B) {
		indirect = (blocks - D_B + I_B - 1) / I_B;
		blocks += indirect;
		if (indirect > 1) {
			indirect = (indirect - 1 + I_B - 1) / I_B;
			blocks += indirect;
			if (indirect > 1)
				blocks++;
		}
	}
	stat->blocks = (BLOCK_SIZE / 512) * blocks;
	stat->blksize = BLOCK_SIZE;
	return 0;
}

int vfs_stat(char *filename, struct kstat *stat)
{
	struct nameidata nd;
	int error;

	error = user_path_walk(filename, &nd);
	if (error)
		return error;

	error = do_revalidate(nd.dentry);
	if (!error) {
		struct inode *inode = nd.dentry->d_inode;
		if (!inode->i_blksize)
			error = getattr_minix(nd.mnt, nd.dentry, stat);
		else
			error = getattr_full(nd.mnt, nd.dentry, stat);
		path_release(&nd);
	}

	return error;
}

int vfs_lstat(char *filename, struct kstat *stat)
{
	struct nameidata nd;
	int error;

	error = user_path_walk_link(filename, &nd);
	if (error)
		return error;

	error = do_revalidate(nd.dentry);
	if (!error) {
		struct inode *inode = nd.dentry->d_inode;
		if (!inode->i_blksize)
			error = getattr_minix(nd.mnt, nd.dentry, stat);
		else
			error = getattr_full(nd.mnt, nd.dentry, stat);
		path_release(&nd);
	}

	return error;
}

int vfs_fstat(int fd, struct kstat *stat)
{
	struct file *file = fget(fd);
	int error;

	if (!file)
		return -EBADF;

	error = do_revalidate(file->f_dentry);
	if (!error) {
		struct inode *inode = file->f_dentry->d_inode;
		if (!inode->i_blksize)
			error = getattr_minix(file->f_vfsmnt, file->f_dentry, stat);
		else
			error = getattr_full(file->f_vfsmnt, file->f_dentry, stat);
		fput(file);
	}
	return error;
}


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
int short_inode_map_initialize(int);
u_short short_inode_map_hashval(ino_t);
void short_inode_map_hash(u_short, ino_t);
void short_inode_map_use(u_short);
ino_t short_inode_map(ino_t);
int short_inode_map_check_idx(u_short);
int short_inode_map_check_lru(void);
int short_inode_map_check_hash(u_short);
int short_inode_map_check(short_inode_map_check_results_t *);


/*
 * Allocate SHORT_INODE_MAP and initialize the linked lists
 */
int
short_inode_map_initialize(int with_lock) {
	int	i;

	if(with_lock)
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

	if(with_lock)
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
			"[%4x] %04x:%08x %4x %4x %4x %4x %4x\n",
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


/*
 * Preliminary /proc/abi - support.
 * Shamelessly copied some parts from scsi_proc.c
 */
#ifdef CONFIG_PROC_FS
static int
abi_gen_proc_write(struct file * file, const char * buf, unsigned long length, void *data) {
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

out:
	
	free_page((unsigned long) buffer);
	return err;
}


static int
abi_proc_info(char *buffer, char **start, off_t offset, int length) {
	int size, len = 0;
	off_t begin = 0;
	off_t pos = 0;

	len += sprintf(buffer + len, "short_inode_mapping: %d\n", short_inode_mapping);

	pos = begin + len;

	*start = buffer + (offset - begin);	/* Start of wanted data */
	len -= (offset - begin);	/* Start slop */
	if (len > length)
		len = length;	/* Ending slop */
	return (len);
}


static int
short_inode_mapping_proc_info(char *buffer, char **start, off_t offset, int length) {
	short_inode_map_check_results_t	results;
	int size, len = 0;
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


static int __init
util_module_init(void)
{
	struct proc_dir_entry *pe;

	/*
	 * This makes /proc/abi and /proc/abi/abi visible.
	 */
#ifdef CONFIG_PROC_FS
	if(! (proc_abi = proc_mkdir("abi", 0))) {
		printk(KERN_ERR "cannot init /proc/abi\n");
		return -ENOMEM;
	}

	if(! (pe = create_proc_info_entry ("abi/abi", 0, 0, abi_proc_info))) {
		printk(KERN_ERR "cannot init /proc/abi/abi\n");
		remove_proc_entry("abi", 0);
		return -ENOMEM;
	}

	pe->write_proc = abi_gen_proc_write;

	if(! (pe = create_proc_info_entry ("abi/short_inode_mapping", 0, 0, short_inode_mapping_proc_info))) {
		printk(KERN_ERR "cannot init /proc/abi/short_inode_mapping\n");
		remove_proc_entry("abi/abi", 0);
		remove_proc_entry("abi", 0);
		return -ENOMEM;
	}

	pe->write_proc = short_inode_mapping_proc_write;
#endif

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
		return short_inode_map_initialize(1);
	else
		return 0;
}


static void __exit
util_module_exit(void)
{
#ifdef CONFIG_PROC_FS
	/* No, we're not here anymore. Don't show the /proc/abi files. */
	remove_proc_entry ("abi/short_inode_mapping", 0);
	remove_proc_entry ("abi/abi", 0);
	remove_proc_entry ("abi", 0);
#endif
	
	if(short_inode_mapping) {
		if(SHORT_INODE_MAP != NULL) {
			vfree(SHORT_INODE_MAP);
			SHORT_INODE_MAP = NULL;
		}
	}
}

module_init(util_module_init);
module_exit(util_module_exit);
