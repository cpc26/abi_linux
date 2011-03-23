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
	{ 0x12da5bb2, "__kmalloc" },
	{ 0x55abbbd5, "send_sig" },
	{ 0x105e2727, "__tracepoint_kmalloc" },
	{ 0xe7fb8388, "setup_arg_pages" },
	{ 0x167e7f9d, "__get_user_1" },
	{ 0x8491221b, "kernel_read" },
	{ 0x9eecde16, "do_brk" },
	{ 0x6436e2a3, "flush_old_exec" },
	{ 0x99bfbe39, "get_unused_fd" },
	{ 0xb72397d5, "printk" },
	{ 0xa1c76e0a, "_cond_resched" },
	{ 0xb4390f9a, "mcount" },
	{ 0x6c2e3320, "strncmp" },
	{ 0xc3aaf0a9, "__put_user_1" },
	{ 0x1e6d26a8, "strstr" },
	{ 0x9e25bb5b, "abi_personality" },
	{ 0x28d761c3, "sys_call" },
	{ 0xd851af78, "up_write" },
	{ 0x45d55543, "down_write" },
	{ 0x21f82b59, "do_mmap_pgoff" },
	{ 0x2e2400f, "kmem_cache_alloc" },
	{ 0xb2fd5ceb, "__put_user_4" },
	{ 0x93fca811, "__get_free_pages" },
	{ 0x5456a608, "unregister_binfmt" },
	{ 0x4302d0eb, "free_pages" },
	{ 0xa071927, "set_binfmt" },
	{ 0x37a0cba, "kfree" },
	{ 0xf27e678, "fd_install" },
	{ 0x6a47571d, "__set_personality" },
	{ 0x33854970, "__register_binfmt" },
	{ 0x9ebff902, "start_thread" },
	{ 0x29c1c74f, "abi_trace_flg" },
	{ 0xc85b4b59, "install_exec_creds" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=abi_lcall,abi_util";


MODULE_INFO(srcversion, "59E33E9C9352471EBEB3C46");
