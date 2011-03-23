#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
 .name = KBUILD_MODNAME,
 .arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0xf13803af, "module_layout" },
	{ 0xb9a12636, "per_cpu__current_task" },
	{ 0xe23d7acb, "up_read" },
	{ 0xb279da12, "pv_lock_ops" },
	{ 0xd0089a36, "abi_sigsuspend" },
	{ 0xeeb6a709, "ibcs_sysconf" },
	{ 0xc7bbbd53, "abi_sigret" },
	{ 0xb4b0ee4e, "down_read" },
	{ 0x7c60d66e, "getname" },
	{ 0xb72397d5, "printk" },
	{ 0xecde1418, "_spin_lock_irq" },
	{ 0xcbd62ee3, "uts_sem" },
	{ 0xa1c76e0a, "_cond_resched" },
	{ 0x2f287f0d, "copy_to_user" },
	{ 0xb4390f9a, "mcount" },
	{ 0x5782b212, "lcall7_dispatch" },
	{ 0x59ac512e, "abi_sigprocmask" },
	{ 0x773ca527, "init_uts_ns" },
	{ 0x28d761c3, "sys_call" },
	{ 0x118f01ea, "putname" },
	{ 0x8ff4079b, "pv_irq_ops" },
	{ 0xb2fd5ceb, "__put_user_4" },
	{ 0xd62c833f, "schedule_timeout" },
	{ 0xb4ae86c2, "fget" },
	{ 0x5a4896a8, "__put_user_2" },
	{ 0x8f9c199c, "__get_user_2" },
	{ 0x7413793a, "EISA_bus" },
	{ 0xd6c963c, "copy_from_user" },
	{ 0x29c1c74f, "abi_trace_flg" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=abi_svr4,abi_lcall,abi_util";


MODULE_INFO(srcversion, "9B7C05A596366DEDA5CD75D");
