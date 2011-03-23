#ident "%W% %G%"

#include <linux/string.h>
#include <linux/fs.h>
#include <linux/slab.h> /* needed by putname macro */

#include <abi/util/trace.h>


static void
print_string(char *buf, char *str)
{
	char *tmp;

	tmp = getname(str);
	if (!IS_ERR(tmp)) {
		/* we are debugging, we don't need to see it all */
		tmp[80] = '\0';
		sprintf(buf, "\"%s\"", tmp);
		putname(tmp);
	}
}

void plist(char *name, char *args, int *list)
{
	char buf[512], *p = buf;

	buf[0] = '\0';
	while (*args) {
		switch (*args++) {
		case 'd':
			sprintf(p, "%d", *list++);
			break;
		case 'o':
			sprintf(p, "0%o", *list++);
			break;
		case 'p':
			sprintf(p, "%p", (void *)(*list++));
			break;
		case '?': 
		case 'x':
			sprintf(p, "0x%x", *list++);
			break;
		case 's':
			print_string(p, (char *)(*list++));
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

