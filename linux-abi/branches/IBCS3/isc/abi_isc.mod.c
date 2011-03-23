#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
 .name = KBUILD_MODNAME,
 .init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
 .exit = cleanup_module,
#endif
 .arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0xf13803af, "module_layout" },
	{ 0xec53015f, "svr4_sysconfig" },
	{ 0x69b9d202, "svr4_xstat" },
	{ 0x5f021865, "abi_brk" },
	{ 0x2831a1e5, "abi_sigfunc" },
	{ 0x92b8a558, "svr4_hrtsys" },
	{ 0xe56ce72e, "abi_wait" },
	{ 0xe3ab1532, "svr4_fpathconf" },
	{ 0xcedf9109, "abi_debug" },
	{ 0xbc89b7d3, "abi_gettime" },
	{ 0x8f8bfbc4, "svr4_xmknod" },
	{ 0xecb63c5a, "svr4_getrlimit" },
	{ 0xba31cc32, "svr4_setegid" },
	{ 0x47ecc1c3, "svr4_getmsg" },
	{ 0xaec1c14e, "abi_exec" },
	{ 0x3bcc0bf2, "abi_sigaction" },
	{ 0x173aeb82, "unregister_exec_domain" },
	{ 0x31b60d07, "svr4_open" },
	{ 0xd0089a36, "abi_sigsuspend" },
	{ 0x4fc1c15a, "abi_mkdir" },
	{ 0x3dc0e554, "abi_kill" },
	{ 0x4ee3a4c6, "abi_procids" },
	{ 0xccdf7b56, "lcall7_syscall" },
	{ 0x8d749693, "v7_utsname" },
	{ 0xeeb6a709, "ibcs_sysconf" },
	{ 0xd05d208e, "svr4_mmap" },
	{ 0xfe136b4f, "abi_getpid" },
	{ 0xb7f5ad1f, "svr4_sysi86" },
	{ 0x2978dfaa, "svr4_fstat" },
	{ 0xf1262302, "abi_pipe" },
	{ 0x3e3f63e5, "svr4_semsys" },
	{ 0x555f13a2, "abi_fork" },
	{ 0x5ae495a0, "svr4_fxstat" },
	{ 0x61d5db42, "svr4_fstatvfs" },
	{ 0xb4390f9a, "mcount" },
	{ 0x5782b212, "lcall7_dispatch" },
	{ 0xcb122fb6, "svr4_sysinfo" },
	{ 0x412e44c2, "svr4_statvfs" },
	{ 0x59ac512e, "abi_sigprocmask" },
	{ 0x3cab0395, "svr4_ioctl" },
	{ 0xa7b4727d, "abi_utsname" },
	{ 0x75895dc7, "svr4_lstat" },
	{ 0x176f9606, "svr4_context" },
	{ 0xa85a53ef, "svr4_statfs" },
	{ 0x85c3f2f1, "abi_getgid" },
	{ 0x69fb05e6, "abi_settime" },
	{ 0x97372b7a, "svr4_getdents" },
	{ 0x4aa59002, "abi_stime" },
	{ 0x7bd75757, "svr4_getpmsg" },
	{ 0xb8ad861d, "register_exec_domain" },
	{ 0xcab3db7c, "svr4_stat" },
	{ 0xadcafe38, "svr4_sigpending" },
	{ 0x2de881dd, "svr4_pathconf" },
	{ 0xdb380518, "svr4_lxstat" },
	{ 0x1eb15e03, "svr4_sysfs" },
	{ 0xb5cd8fa7, "abi_read" },
	{ 0xd383fc4a, "svr4_setrlimit" },
	{ 0xb175d33a, "abi_time" },
	{ 0x74bec825, "socksys_syscall" },
	{ 0x6674d51c, "abi_getuid" },
	{ 0x5a12bf51, "svr4_seteuid" },
	{ 0x9dfb77d1, "svr4_msgsys" },
	{ 0x59307f26, "svr4_fstatfs" },
	{ 0xd99cd0d8, "svr4_shmsys" },
	{ 0x546fab6a, "svr4_ulimit" },
	{ 0xc68fa320, "abi_utime" },
	{ 0x609fa4bc, "svr4_fcntl" },
	{ 0x1ba587a9, "svr4_putmsg" },
	{ 0xd6ec9657, "svr4_putpmsg" },
	{ 0x6ac149f0, "svr4_mknod" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=abi_svr4,abi_lcall";


MODULE_INFO(srcversion, "074ED2255C64D27B8FE88CD");
