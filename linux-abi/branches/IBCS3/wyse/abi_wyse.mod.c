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
	{ 0xb9a12636, "per_cpu__current_task" },
	{ 0x5f021865, "abi_brk" },
	{ 0x2831a1e5, "abi_sigfunc" },
	{ 0xe23d7acb, "up_read" },
	{ 0xe56ce72e, "abi_wait" },
	{ 0xcedf9109, "abi_debug" },
	{ 0x47ecc1c3, "svr4_getmsg" },
	{ 0xaec1c14e, "abi_exec" },
	{ 0x173aeb82, "unregister_exec_domain" },
	{ 0x31b60d07, "svr4_open" },
	{ 0x4fc1c15a, "abi_mkdir" },
	{ 0x3dc0e554, "abi_kill" },
	{ 0x6729d3df, "__get_user_4" },
	{ 0x4ee3a4c6, "abi_procids" },
	{ 0xccdf7b56, "lcall7_syscall" },
	{ 0x8d749693, "v7_utsname" },
	{ 0xfe136b4f, "abi_getpid" },
	{ 0xb4b0ee4e, "down_read" },
	{ 0xb7f5ad1f, "svr4_sysi86" },
	{ 0x2978dfaa, "svr4_fstat" },
	{ 0xf1262302, "abi_pipe" },
	{ 0x3e3f63e5, "svr4_semsys" },
	{ 0x555f13a2, "abi_fork" },
	{ 0xb72397d5, "printk" },
	{ 0xcbd62ee3, "uts_sem" },
	{ 0xa1c76e0a, "_cond_resched" },
	{ 0x2f287f0d, "copy_to_user" },
	{ 0xb4390f9a, "mcount" },
	{ 0x5782b212, "lcall7_dispatch" },
	{ 0x3cab0395, "svr4_ioctl" },
	{ 0x773ca527, "init_uts_ns" },
	{ 0x28d761c3, "sys_call" },
	{ 0x75895dc7, "svr4_lstat" },
	{ 0xa85a53ef, "svr4_statfs" },
	{ 0x85c3f2f1, "abi_getgid" },
	{ 0x97372b7a, "svr4_getdents" },
	{ 0x4aa59002, "abi_stime" },
	{ 0xb8ad861d, "register_exec_domain" },
	{ 0xb2fd5ceb, "__put_user_4" },
	{ 0xcab3db7c, "svr4_stat" },
	{ 0x1eb15e03, "svr4_sysfs" },
	{ 0xb5cd8fa7, "abi_read" },
	{ 0xb175d33a, "abi_time" },
	{ 0x6674d51c, "abi_getuid" },
	{ 0x9dfb77d1, "svr4_msgsys" },
	{ 0x59307f26, "svr4_fstatfs" },
	{ 0xd99cd0d8, "svr4_shmsys" },
	{ 0x546fab6a, "svr4_ulimit" },
	{ 0xc68fa320, "abi_utime" },
	{ 0x609fa4bc, "svr4_fcntl" },
	{ 0x1ba587a9, "svr4_putmsg" },
	{ 0x29c1c74f, "abi_trace_flg" },
	{ 0x6ac149f0, "svr4_mknod" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=abi_svr4,abi_lcall,abi_util";


MODULE_INFO(srcversion, "388ED742C3C18B3F2C19412");
