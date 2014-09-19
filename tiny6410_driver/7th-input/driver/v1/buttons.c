/*
	参考输入子系统编写输入设备驱动
	基本框架
	Author:jefby
	Email:jef199006@gmail.com

*/
#include <linux/module.h>//MODULE_AUTHOR,MODULE_LICENSE
#include <linux/init.h>//module_init/exit



static int input_buttons_init(void)
{

	printk(KERN_ALERT "input_buttons_init\n");
	return 0;
}

static void input_buttons_exit(void)
{
	printk(KERN_ALERT "input_buttons_exit\n");
}

module_init(input_buttons_init);//入口
module_exit(input_buttons_exit);//出口


MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("jefby");
MODULE_DESCRIPTION("Tiny6410 key driver with input subsystem.");



















