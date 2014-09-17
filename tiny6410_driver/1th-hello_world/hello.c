#include <linux/init.h>
#include <linux/module.h>


//#define GPKDAT 

MODULE_LICENSE("GPL");
MODULE_AUTHOR("jefby");
static int __init hello_init(void)
{
	printk("Tiny 6410 hello module init.\n");
	return 0;
}

static void __exit hello_exit(void)
{
	printk("Tiny 6410 leds module exit.\n");
}


module_init(hello_init);
module_exit(hello_exit);


