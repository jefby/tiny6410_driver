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
	{ 0xb0499372, "class_destroy" },
	{ 0x737a1c81, "device_destroy" },
	{ 0x3ce76ee4, "device_create" },
	{ 0x2ed4ada9, "__class_create" },
	{ 0xeee994bc, "cdev_add" },
	{ 0x951e644d, "cdev_init" },
	{ 0xe10fcab1, "cdev_alloc" },
	{ 0x29537c9e, "alloc_chrdev_region" },
	{ 0xd8e484f0, "register_chrdev_region" },
	{ 0x2e5810c6, "__aeabi_unwind_cpp_pr1" },
	{ 0x27e1a049, "printk" },
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

