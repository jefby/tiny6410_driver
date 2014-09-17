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
	{ 0x9e4d2947, "module_layout" },
	{ 0x7485e15e, "unregister_chrdev_region" },
	{ 0x69332643, "cdev_del" },
	{ 0xeee994bc, "cdev_add" },
	{ 0x951e644d, "cdev_init" },
	{ 0xe10fcab1, "cdev_alloc" },
	{ 0x29537c9e, "alloc_chrdev_region" },
	{ 0xd8e484f0, "register_chrdev_region" },
	{ 0x59237210, "__wake_up" },
	{ 0x27e1a049, "printk" },
	{ 0x859c6dc7, "request_threaded_irq" },
	{ 0xc8b57c27, "autoremove_wake_function" },
	{ 0xfa2a45e, "__memzero" },
	{ 0x67c2fa54, "__copy_to_user" },
	{ 0x87280639, "finish_wait" },
	{ 0x1000e51, "schedule" },
	{ 0x33333f72, "prepare_to_wait" },
	{ 0x5f754e5a, "memset" },
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
	{ 0xf20dabd8, "free_irq" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

