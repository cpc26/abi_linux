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
	{ 0x9bb69589, "kmalloc_caches" },
	{ 0xd0d8621b, "strlen" },
	{ 0x173aeb82, "unregister_exec_domain" },
	{ 0x105e2727, "__tracepoint_kmalloc" },
	{ 0x6729d3df, "__get_user_4" },
	{ 0x3c2c5af5, "sprintf" },
	{ 0x24428be5, "strncpy_from_user" },
	{ 0x41344088, "param_get_charp" },
	{ 0xb72397d5, "printk" },
	{ 0xa1c76e0a, "_cond_resched" },
	{ 0xb4390f9a, "mcount" },
	{ 0xd851af78, "up_write" },
	{ 0x45d55543, "down_write" },
	{ 0x21f82b59, "do_mmap_pgoff" },
	{ 0x2e2400f, "kmem_cache_alloc" },
	{ 0xb8ad861d, "register_exec_domain" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0x5456a608, "unregister_binfmt" },
	{ 0x6ad065f4, "param_set_charp" },
	{ 0x37a0cba, "kfree" },
	{ 0x6a47571d, "__set_personality" },
	{ 0x33854970, "__register_binfmt" },
	{ 0xc965645a, "plist" },
	{ 0x29c1c74f, "abi_trace_flg" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=abi_util";


MODULE_INFO(srcversion, "FBF325F875EA6D33EE50E67");
