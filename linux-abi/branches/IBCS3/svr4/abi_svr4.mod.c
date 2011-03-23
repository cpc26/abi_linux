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
	{ 0x6bc3fbc0, "__unregister_chrdev" },
	{ 0xb9a12636, "per_cpu__current_task" },
	{ 0x9bb69589, "kmalloc_caches" },
	{ 0x12da5bb2, "__kmalloc" },
	{ 0xe23d7acb, "up_read" },
	{ 0xb279da12, "pv_lock_ops" },
	{ 0x5733c29c, "vfs_readdir" },
	{ 0xd0d8621b, "strlen" },
	{ 0x1a75caa3, "_read_lock" },
	{ 0x326df9e1, "vfs_stat" },
	{ 0x2d37342e, "cpu_online_mask" },
	{ 0x105e2727, "__tracepoint_kmalloc" },
	{ 0x6729d3df, "__get_user_4" },
	{ 0x245b0a5c, "__register_chrdev" },
	{ 0x77e394df, "__kill_fasync" },
	{ 0xcb6beb40, "hweight32" },
	{ 0x167e7f9d, "__get_user_1" },
	{ 0x3c2c5af5, "sprintf" },
	{ 0xb3a307c6, "si_meminfo" },
	{ 0xb4b0ee4e, "down_read" },
	{ 0xe2d5255a, "strcmp" },
	{ 0x8b18496f, "__copy_to_user_ll" },
	{ 0x41344088, "param_get_charp" },
	{ 0xb8e7ce2c, "__put_user_8" },
	{ 0x7c60d66e, "getname" },
	{ 0xb72397d5, "printk" },
	{ 0xecde1418, "_spin_lock_irq" },
	{ 0xcbd62ee3, "uts_sem" },
	{ 0xa1c76e0a, "_cond_resched" },
	{ 0x2f287f0d, "copy_to_user" },
	{ 0xb4390f9a, "mcount" },
	{ 0x614f8d56, "pdump" },
	{ 0xc3aaf0a9, "__put_user_1" },
	{ 0x3daa69da, "vfs_lstat" },
	{ 0xd3e69dfb, "poll_freewait" },
	{ 0x773ca527, "init_uts_ns" },
	{ 0x28d761c3, "sys_call" },
	{ 0xd851af78, "up_write" },
	{ 0x45d55543, "down_write" },
	{ 0xeda84435, "fput" },
	{ 0x7a0073b7, "poll_initwait" },
	{ 0xf6ff99eb, "__task_pid_nr_ns" },
	{ 0x4020a4b6, "short_inode_map" },
	{ 0x118f01ea, "putname" },
	{ 0x21f82b59, "do_mmap_pgoff" },
	{ 0xf2e6135a, "find_vma" },
	{ 0x7dceceac, "capable" },
	{ 0x7171121c, "overflowgid" },
	{ 0x2e2400f, "kmem_cache_alloc" },
	{ 0x8ff4079b, "pv_irq_ops" },
	{ 0xb2fd5ceb, "__put_user_4" },
	{ 0x93fca811, "__get_free_pages" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0x4292364c, "schedule" },
	{ 0xfb6af58d, "recalc_sigpending" },
	{ 0x7362dd1e, "vfs_fstat" },
	{ 0x86b3eed5, "vfs_statfs" },
	{ 0x9335317e, "user_path_at" },
	{ 0x390543a1, "path_put" },
	{ 0x6ad065f4, "param_set_charp" },
	{ 0x4302d0eb, "free_pages" },
	{ 0x642e54ac, "__wake_up" },
	{ 0x1584eac8, "init_pid_ns" },
	{ 0x37a0cba, "kfree" },
	{ 0xb4ae86c2, "fget" },
	{ 0x5a4896a8, "__put_user_2" },
	{ 0x8b618d08, "overflowuid" },
	{ 0x8f9c199c, "__get_user_2" },
	{ 0xd6c963c, "copy_from_user" },
	{ 0x29c1c74f, "abi_trace_flg" },
	{ 0xe914e41e, "strcpy" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=abi_lcall,abi_util";


MODULE_INFO(srcversion, "E0BAE4B17457FBF54D93CC3");
