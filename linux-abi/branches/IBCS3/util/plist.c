//#ident "%W% %G%"

#include "../include/util/i386_std.h"
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/slab.h> /* needed by putname macro */
#include <linux/sched.h> /* needed by current-> in __abi_trace() macro */
#include <linux/personality.h>
#include <asm/uaccess.h>

#include "../include/util/trace.h"


void plist(char *name, char *args, int *list)
{
	char buf[512], *p;
	unsigned long addr;

	buf[0] = '\0';
	p = buf;
	while (*args) {
//printk("Arg=%c,List=%lX\n",*args,*list);
		switch (*args++) {
		case 'd':
			sprintf(p, "%d", *list++);
			break;
		case 'o':
			sprintf(p, "0%o", (*list++)&0xFF);
			break;
		case 'p':
			addr = (unsigned long)(*list++);
			addr = (addr) & 0xFFFFFFFF;
			sprintf(p, "%8p", (void *)addr);
			break;
		case '?':
		case 'x':
			sprintf(p, "0x%x", *list++);
			break;
		case 's':
			addr = (unsigned long)(*list++);
			addr = (addr) & 0xFFFFFFFF;
			strcat(p,"\"");p++;
			strncpy_from_user(p,(const char __user *)addr,256);
			strcat(p,"\"");p++;
			break;
		default:
			sprintf(p, "?%c%c?", '%', args[-1]);
			break;
		}

		while (*p)
			++p;
		if (*args) {
			*p++ = ',';
			*p++ = ' ';
			*p = '\0';
		}
	}
	__abi_trace("%s(%s)\n", name, buf);
}
#define MAX_DUMP 680
int
pdump(char __user *s, int l)
{
  char *p, buf[MAX_DUMP*3], c;
  int i, j, k;
  if (l<=0) return 0;
  if (l>MAX_DUMP) l = MAX_DUMP;
  p=buf;
  for(i=0; i<l; i++) {
    /*get_user(c,s+i);*/ c=s[i];
    j = (c & 0xF0) >> 4;
    k = c & 0x0F;
    if (j>9) p[0] = 55 + j;
     else p[0] = 48 + j;
    if (k>9) p[1] = 55 + k;
     else p[1] = 48 + k;
    if (((i+1) % sizeof(long)) == 0) p[2] = ' ';
    else p[2]='.';
    p += 3;
   }
  p[0]='\0';
  i=printk("%s\n",buf); 
  return i;
}
u_int abi_trace_flg;

EXPORT_SYMBOL(plist);
EXPORT_SYMBOL(pdump);
EXPORT_SYMBOL(abi_trace_flg);
