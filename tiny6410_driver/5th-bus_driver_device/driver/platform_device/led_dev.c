#include <linux/init.h>
#include <linux/module.h>

#include <linux/platform_device.h>


//分配/设置/注册一个platform_device设备
void	led_platdev_release(struct device *dev)
{
	printk(KERN_ALERT "led_platdev_release.\n");
}

//资源
static struct resource tiny6410_led_resource[] = {
	[0] = {
		.start	= 0x7F008800,//gpkcon的起始地址
		.end	= 0x7F008800 + 16 -1,//结束地址
		.flags	= IORESOURCE_MEM
	},
	[1] = {
		.start	= 5,//表示第4位
		.end	= 5,
		.flags	= IORESOURCE_IRQ
	}
};

//平台设备
static struct platform_device led_platdevice = {
	.name		= "platdev_led",//名称
	.id		= -1,
	.num_resources	= ARRAY_SIZE(tiny6410_led_resource),//资源个数
	.resource	= tiny6410_led_resource,//资源
	.dev = {//struct device dev字段
		.release = led_platdev_release,//设置release函数为NULL
	}
};

static int led_platdevice_init(void)
{
	platform_device_register(&led_platdevice);
	printk(KERN_ALERT "led_platdevice_init.\n");
	return 0;
}

static void led_platdevice_exit(void)
{
	platform_device_unregister(&led_platdevice);
	printk(KERN_ALERT "led_platdevice_exit.\n");
}

module_init(led_platdevice_init);
module_exit(led_platdevice_exit);
MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("jefby");



