#include <linux/kernel.h>
#include <linux/net.h>
#include <linux/socket.h>
#include <abi/socket.h>
#include <linux/signal.h>
#include <abi/map.h>
#define __NO_VERSION__
 #include <linux/module.h>

struct map_segment abi_sockopt_map[] =  {
	{ 0x0001, 0x0001, (char *)SO_DEBUG },
	{ 0x0002, 0x0002, (char *)SO_ACCEPTCONN },
	{ 0x0004, 0x0004, (char *)SO_REUSEADDR },
	{ 0x0008, 0x0008, (char *)SO_KEEPALIVE },
	{ 0x0010, 0x0010, (char *)SO_DONTROUTE },
	{ 0x0020, 0x0020, (char *)SO_BROADCAST },
	{ 0x0040, 0x0040, (char *)SO_USELOOPBACK },
	{ 0x0080, 0x0080, (char *)SO_LINGER },
	{ 0x0100, 0x0100, (char *)SO_OOBINLINE },
	{ 0x0200, 0x0200, (char *)SO_ORDREL },
	{ 0x0400, 0x0400, (char *)SO_IMASOCKET },
	{ 0x1001, 0x1001, (char *)SO_SNDBUF },
	{ 0x1002, 0x1002, (char *)SO_RCVBUF },
	{ 0x1003, 0x1003, (char *)SO_SNDLOWAT },
	{ 0x1004, 0x1004, (char *)SO_RCVLOWAT },
	{ 0x1005, 0x1005, (char *)SO_SNDTIMEO },
	{ 0x1006, 0x1006, (char *)SO_RCVTIMEO },
	{ 0x1007, 0x1007, (char *)SO_ERROR },
	{ 0x1008, 0x1008, (char *)SO_TYPE },
	{ 0x1009, 0x1009, (char *)SO_PROTOTYPE },
	{ -1 }
};


struct map_segment abi_af_map[] =  {
	/* The first three entries (AF_UNSPEC, AF_UNIX and AF_INET)
	 * are identity mapped. All others aren't available under
	 * Linux, nor are Linux's AF_AX25 and AF_IPX available from
	 * SCO as far as I know.
	 */
	{ 0, 2, NULL },
	{ -1 }
};

EXPORT_SYMBOL(abi_af_map);
EXPORT_SYMBOL(abi_sockopt_map);
