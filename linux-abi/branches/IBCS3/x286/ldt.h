#ifndef _X286_LDT_H_
#define _X286_LDT_H_

struct segment_descriptor {
	unsigned lolimit:16; 	/* segment extent (lsb) */
	unsigned lobase1:16;	/* segment base address (lsb) */
	unsigned lobase2:8;
	unsigned type:5;	/* segment type */
	unsigned dpl:2;		/* segment descriptor priority level */
	unsigned p:1;		/* segment descriptor present */
	unsigned hilimit:4;	/* segment extent (msb) */
	unsigned user:1;
	unsigned x:1;
	unsigned def32:1;	/* default 32 vs 16 bit size */
	unsigned gran:1;	/* limit granularity (byte/page units)*/
	unsigned hibase:8;	/* segment base address  (msb) */
};

struct gate_descriptor {
	unsigned looffset:16;	/* gate offset (lsb) */
	unsigned selector:16;	/* gate segment selector */
	unsigned stkcpy:5;	/* number of stack wds to cpy */
	unsigned xx:3;		/* unused */
	unsigned type:5;	/* segment type */
	unsigned dpl:2;		/* segment descriptor priority level */
	unsigned p:1;		/* segment descriptor present */
	unsigned hioffset:16;	/* gate offset (msb) */
};

struct anon_descriptor {
	unsigned junk1:16;
	unsigned junk2:16;
	unsigned junk3:8;
	unsigned type:5;
	unsigned dpl:2;
	unsigned p:1;
	unsigned junk4:16;
};

typedef union descriptor {
	struct segment_descriptor sd;
	struct gate_descriptor gd;
	struct anon_descriptor ad;
} descriptor_t;


#define	D_NULL		 0	/* system null */
#define	D_286TSS	 1	/* system 286 TSS available */
#define	D_LDT		 2	/* system local descriptor table */
#define	D_286BSY	 3	/* system 286 TSS busy */
#define	D_286CGT	 4	/* system 286 call gate */
#define	D_TASKGT	 5	/* system task gate */
#define	D_286IGT	 6	/* system 286 interrupt gate */
#define	D_286TGT	 7	/* system 286 trap gate */
#define	D_NULL2		 8	/* system null again */
#define	D_386TSS	 9	/* system 386 TSS available */
#define	D_NULL3		10	/* system null again */
#define	D_386BSY	11	/* system 386 TSS busy */
#define	D_386CGT	12	/* system 386 call gate */
#define	D_NULL4		13	/* system null again */
#define	D_386IGT	14	/* system 386 interrupt gate */
#define	D_386TGT	15	/* system 386 trap gate */
#define	D_MEMRO		16	/* memory read only */
#define	D_MEMROA	17	/* memory read only accessed */
#define	D_MEMRW		18	/* memory read write */
#define	D_MEMRWA	19	/* memory read write accessed */
#define	D_MEMROD	20	/* memory read only expand dwn limit */
#define	D_MEMRODA	21	/* memory read only expand dwn limit accessed */
#define	D_MEMRWD	22	/* memory read write expand dwn limit */
#define	D_MEMRWDA	23	/* memory read write expand dwn limit accessed */
#define	D_MEMX		24	/* memory execute only */
#define	D_MEMXA		25	/* memory execute only accessed */
#define	D_MEMXR		26	/* memory execute read */
#define	D_MEMXRA	27	/* memory execute read accessed */
#define	D_MEMXC		28	/* memory execute only conforming */
#define	D_MEMXAC	29	/* memory execute only accessed conforming */
#define	D_MEMXRC	30	/* memory execute read conforming */
#define	D_MEMXRAC	31	/* memory execute read accessed conforming */


struct ldt_desc {
	unsigned long base;
	long limit;
	unsigned char type, dpl;
	unsigned short rlimit;
};
#define LDT_ENTRY_SIZE	8
struct user_ldt {
	unsigned int  entry_number;
	unsigned long base_addr;
	unsigned int  limit;
	unsigned int  seg_32bit:1;
	unsigned int  contents:2;
	unsigned int  read_exec_only:1;
	unsigned int  limit_in_pages:1;
	unsigned int  seg_not_present:1;
	unsigned int  useable:1;
};
#define LDT_DATA	0
#define LDT_STACK	1
#define LDT_CODE	2

#define MAX_SEGMENTS 256

extern struct ldt_desc ldt[MAX_SEGMENTS];
extern int base_desc, last_desc;

void ldt_init(void);
int modify_ldt(int, void *, unsigned long);

#endif /* _X286_LDT_H */
